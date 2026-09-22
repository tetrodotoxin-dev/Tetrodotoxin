// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/rpc/executor.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/utility/table.hpp"

#include "puffer/lsp/methods.hpp"
#include "puffer/lsp/rpc/frame_reader.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Puffer;

template <const auto& dispatch_table>
auto Lsp::Rpc::Executor<dispatch_table>::execute(View::Bytes pipe_name)
    -> void {
  if (!create_connection(pipe_name)) {
    Diagnostics::Log::error("RPC server entered an invalid state."_view);
    return;
  }

  Diagnostics::Log::info("TTX RPC service is running."_view);
  process_events();

  Diagnostics::Log::info("TTX RPC service is closing its socket."_view);
  close_connection();
  close(socket_descriptor);
  socket_descriptor = -1;
}

template <const auto& dispatch_table>
auto Lsp::Rpc::Executor<dispatch_table>::create_connection(
    View::Bytes pipe_name) -> Bool {
  socket_descriptor = socket(AF_FILE, SOCK_STREAM, 0);
  if (socket_descriptor == -1) {
    Diagnostics::Log::error("Failed to create the RPC socket."_view);
    return False;
  }

  sockaddr_un address = {};
  address.sun_family = AF_UNIX;

  Count path_length =
      Math::min(pipe_name.get_size(), Count(sizeof(address.sun_path) - 1));
  Data::copy(
      Data::cast<U8>(address.sun_path), pipe_name.get_data(), path_length);
  address.sun_path[path_length] = '\0';

  auto connect_result =
      connect(socket_descriptor, (sockaddr*)&address, sizeof(address));
  if (connect_result == -1) {
    Diagnostics::Log::Message<128> error_message(
        Diagnostics::Log::Level::Error);
    error_message << "Failed to connect to pipe at "_view << pipe_name;
    close(socket_descriptor);
    socket_descriptor = -1;
    return False;
  }

  connection_open = True;
  return True;
}

template <const auto& dispatch_table>
auto Lsp::Rpc::Executor<dispatch_table>::write_jsonrpc_frame(View::Bytes view)
    -> void {
  if (!connection_open) {
    return;
  }

  Count bytes_written = 0;
  while (bytes_written < view.get_size()) {
    S64 bytes = write(
        socket_descriptor, view.get_data() + bytes_written,
        view.get_size() - bytes_written);
    if (bytes <= 0) {
      Diagnostics::Log::error(
          "Writing RPC frame to socket failed, closing connection"_view);
      close_connection();
      return;
    }
    bytes_written += Count(bytes);
  }
}

template <const auto& dispatch_table>
auto Lsp::Rpc::Executor<dispatch_table>::write_response(
    View::Bytes json_response) -> void {
  Static::Bytes<256> header_buffer;
  Writer::Textual content_length(header_buffer);
  content_length << "Content-Length: "_view << json_response.get_size()
                 << "\r\n\r\n"_view;

  write_jsonrpc_frame(content_length);
  write_jsonrpc_frame(json_response);
}

template <const auto& dispatch_table>
auto Lsp::Rpc::Executor<dispatch_table>::process_message(
    Allocator::Arena& arena,
    View::Bytes frame) -> void {
  Message message(arena, frame);
  if (!message.is_valid()) {
    Diagnostics::Log::warning("RPC message could not be decoded."_view);
    return;
  }

  auto method_name = message.get_method();
  using DispatchTable = Table<Lsp::Rpc::DispatchFunc, dispatch_table>;
  auto method = DispatchTable::find(method_name);
  if (!method) {
    // JSON RPC notifications never receive a response, and the LSP reserves
    // `$/*` methods for protocol notifications that a server may ignore.
    if (!message.expects_response()) {
      return;
    }

    Diagnostics::Log::Message<128> warning_message(
        Diagnostics::Log::Level::Warning);
    warning_message << "Unregistered RPC method `"_view << method_name
                    << "`"_view;

    auto error_response = message.report_error(warning_message.get_message());
    write_response(error_response.format(message.get_arena()));
    return;
  }

  {
    Diagnostics::Log::Message<128> accepted_message(
        Diagnostics::Log::Level::Info);
    accepted_message << "Job Accepted: RPC method `"_view << method_name
                     << "`"_view;
  }

  auto response = (*method)(documents, message);
  if (message.expects_response() || !response.is_null()) {
    write_response(response.format(message.get_arena()));
  }
}

template <const auto& dispatch_table>
auto Lsp::Rpc::Executor<dispatch_table>::process_events() -> void {
  constexpr Count chunk_size = 1 << 16;
  Static::Bytes<chunk_size> chunk;
  FrameReader reader;
  Allocator::Arena arena;
  while (connection_open) {
    const auto bytes_read =
        read(socket_descriptor, chunk.get_data(), chunk.get_size());
    if (bytes_read < 0) {
      Diagnostics::Log::error("Error while reading from pipe"_view);
      close_connection();
      return;
    }

    if (bytes_read == 0) {
      Diagnostics::Log::info("LSP pipe was closed by the client"_view);
      close_connection();
      return;
    }

    reader.receive(chunk.slice(0, Count(bytes_read)));
    View::Bytes message = reader.next_message();
    while (!message.is_empty()) {
      process_message(arena, message);
      reader.consume_message();
      arena.reset();
      message = reader.next_message();
    }
  }
}

template <const auto& dispatch_table>
auto Lsp::Rpc::Executor<dispatch_table>::close_connection() -> void {
  connection_open = False;
}

// The executor is templated over the method table so dispatch stays a static
// lookup. Puffer owns the single concrete instantiation here instead of
// carrying a runtime registration path through every server startup.
template class Lsp::Rpc::Executor<Lsp::method_table>;
