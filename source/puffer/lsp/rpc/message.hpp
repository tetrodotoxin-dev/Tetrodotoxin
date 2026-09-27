// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/serialization/json/node.hpp"

namespace Puffer::Lsp::Rpc {

using Response = Perimortem::Serialization::Json::Node;

class Message {
 public:
  Message(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source);

  auto report_error(Perimortem::Core::View::Bytes error) const -> Response;
  auto report_result(const Response& result) const -> Response;

  auto is_valid() const -> Bool;
  auto expects_response() const -> Bool;

  auto get_method() const -> Perimortem::Core::View::Bytes;
  auto get_params() const -> const Perimortem::Serialization::Json::Node&;
  auto get_arena() const -> Perimortem::Memory::Allocator::Arena&;

 private:
  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Serialization::Json::Node parsed;
  Perimortem::Serialization::Json::Node call_params;
};

}  // namespace Puffer::Lsp::Rpc
