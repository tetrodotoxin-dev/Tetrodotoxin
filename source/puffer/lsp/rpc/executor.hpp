// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "puffer/lsp/documents.hpp"
#include "puffer/lsp/rpc/message.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"

namespace Puffer::Lsp::Rpc {

using DispatchFunc = Response (*)(Documents&, const Message&);

template <const auto& dispatch_table>
class Executor {
 public:
  constexpr Executor(Tetrodotoxin::Package::Repository::Repository& repository)
      : documents(repository) {}

  auto execute(Perimortem::Core::View::Bytes pipe_name) -> void;

 private:
  auto create_connection(Perimortem::Core::View::Bytes pipe_name) -> Bool;
  auto write_jsonrpc_frame(Perimortem::Core::View::Bytes view) -> void;
  auto write_response(Perimortem::Core::View::Bytes json_response) -> void;
  auto process_message(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes frame) -> void;
  auto process_events() -> void;
  auto close_connection() -> void;

  Documents documents;
  S32 socket_descriptor = -1;
  Bool connection_open = False;
};

}  // namespace Puffer::Lsp::Rpc
