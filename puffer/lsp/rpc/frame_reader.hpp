// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Puffer::Lsp::Rpc {

class FrameReader {
 public:
  auto receive(Perimortem::Core::View::Bytes bytes) -> void;
  auto next_message() -> Perimortem::Core::View::Bytes;
  auto consume_message() -> void;

 private:
  auto read_header() -> Bool;

  Perimortem::Memory::Dynamic::Bytes data_stream;
  U64 data_bytes_to_read = 0;
  Bool header_found = False;
};

}  // namespace Puffer::Lsp::Rpc
