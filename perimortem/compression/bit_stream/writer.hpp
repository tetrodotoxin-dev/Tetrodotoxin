// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::Compression::BitStream {

// A bit writer for DEFLATE packed streams which dynamically manages a buffer
// compared to the `Access::Bytes` model used by `Core::Writer`.
//
// Each byte is filled starting from its least significant bit, and Huffman
// codes are written from the most significant bit to match canonical decode
// order.
//
// Internally a 64 bit accumulator buffers bits until complete bytes can be
// flushed to the output in one store.
//
// Huffman::encode_symbol returns pre reversed codes, so write_code is
// a zero overhead alias for write_bits.
class Writer {
 public:
  Writer(Memory::Dynamic::Bytes& output) : output(output) {}

  // Write `length` bits of `value` least significant bit first (extra bits,
  // block headers).
  auto write_bits(U32 value, Count length) -> void;
  // Write a pre reversed Huffman code returned by `Huffman::encode_symbol`.
  auto write_code(U32 code, Count length) -> void;
  auto flush() -> void;

 private:
  auto drain() -> void;

  Memory::Dynamic::Bytes& output;
  U64 accumulator = 0;
  Count bits = 0;
};

}  // namespace Perimortem::Compression::BitStream
