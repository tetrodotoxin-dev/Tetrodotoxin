// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/rpc/message.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

using namespace Puffer;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

Lsp::Rpc::Message::Message(
    Perimortem::Memory::Allocator::Arena& arena,
    Perimortem::Core::View::Bytes source)
    : arena(arena) {
  parsed.parse(arena, source);
  call_params = parsed["params"_view];
}

auto Lsp::Rpc::Message::is_valid() const -> Bool {
  return !parsed.is_null() && !parsed["method"_view].get_string().is_empty() &&
         !parsed["jsonrpc"_view].get_string().is_empty() &&
         (expects_response() ? parsed["id"_view].is_number() : True);
}

auto Lsp::Rpc::Message::expects_response() const -> Bool {
  return parsed.contains("id"_view);
}

auto Lsp::Rpc::Message::get_method() const -> Perimortem::Core::View::Bytes {
  return parsed["method"_view].decode_string(arena);
}

auto Lsp::Rpc::Message::get_params() const
    -> const Perimortem::Serialization::Json::Node& {
  return call_params;
}

auto Lsp::Rpc::Message::get_arena() const
    -> Perimortem::Memory::Allocator::Arena& {
  return arena;
}

auto Lsp::Rpc::Message::report_error(Perimortem::Core::View::Bytes error) const
    -> Lsp::Rpc::Response {
  Managed::Bytes sanitized(arena);
  sanitized.proxy(error);
  sanitized.convert('"', '`');
  return Json::Blueprint{
    {
      {"jsonrpc"_view, parsed["jsonrpc"_view].get_string()},
      {"id"_view, parsed["id"_view].get_number()},
      {"error"_view, sanitized.get_view()},
    }}.construct(arena);
}

auto Lsp::Rpc::Message::report_result(const Response& result) const
    -> Lsp::Rpc::Response {
  return Json::Blueprint{
    {
      {"jsonrpc"_view, parsed["jsonrpc"_view].get_string()},
      {"id"_view, parsed["id"_view].get_number()},
      {"result"_view, result},
    }}.construct(arena);
}
