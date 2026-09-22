// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Compression::BitStream {

// A bit reader for DEFLATE packed streams.
//
// Each byte is consumed starting from its least significant bit, and Huffman
// codes accumulate from the most significant bit to match canonical decode
// order.
//
// Internally a 64 bit fill buffer is kept loaded from the stream so peek_code /
// advance_bits require no memory access in the common case. `fill()` is called
// lazily when the buffer drops below the requested bit count.
class Reader {
 public:
  constexpr Reader(Core::View::Bytes source) : data(source) {}

  auto read_bit() -> Bool;
  auto read_code(Count count) -> U32;
  auto read_raw_bytes(Count count) -> Core::View::Bytes;
  auto peek_code(Count count) -> U32;

  constexpr auto advance_bits(Count count) -> void {
    buffer >>= count;
    buffered_bits -= count;
  }

  constexpr auto is_valid() const -> Bool { return !invalid_stream; }
  constexpr auto invalidate() -> void { invalid_stream = True; }

 private:
  auto fill() -> void;

  Core::View::Bytes data;
  U64 buffer = 0;
  Count buffered_bits = 0;
  Count byte_position = 0;
  Bool invalid_stream = False;
};

}  // namespace Perimortem::Compression::BitStream
