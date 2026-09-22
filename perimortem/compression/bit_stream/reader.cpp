// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/compression/bit_stream/reader.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem;

// Loads bytes from the stream into buffer until it buffers at least 57 bits or
// the stream is exhausted. 57 bits guarantees a 9 bit peek after consuming up
// to a 48 bit code + extra bits pair without a second fill.
auto Compression::BitStream::Reader::fill() -> void {
  const Count size = data.get_size();
  const U8* source_pointer = data.get_data();

  // Fast path: enough bytes remain for a single unaligned 64 bit load.
  // On little endian hosts (x86) stream byte order matches load order directly.
  //
  // TODO: Support Big endian at some point.
  if (byte_position + Count(sizeof(U64)) <= size) {
    U64 word = *Data::cast<const U64>(source_pointer + byte_position);
    Count consumed = (64 - buffered_bits) >> 3;
    buffer |= word << buffered_bits;
    byte_position += consumed;
    buffered_bits += consumed * 8;
    return;
  }

  // Slow path: near stream end we need to load byte by byte.
  while (buffered_bits <= 56 && byte_position < size) {
    buffer |= U64(source_pointer[byte_position++]) << buffered_bits;
    buffered_bits += 8;
  }
}

auto Compression::BitStream::Reader::read_bit() -> Bool {
  if (buffered_bits == 0) {
    fill();
    if (buffered_bits == 0) [[unlikely]] {
      invalid_stream = True;
      return False;
    }
  }

  // Load the lowest bit of the buffer.
  const Bool bit = buffer & U64(1);
  advance_bits(1);
  return bit;
}

auto Compression::BitStream::Reader::read_code(Count count) -> U32 {
  if (buffered_bits < count) {
    fill();
    if (buffered_bits < count) [[unlikely]] {
      invalid_stream = True;
      return 0;
    }
  }

  const U32 mask = ((U32(1) << count) - U32(1));
  const U32 result = U32(buffer) & mask;
  advance_bits(count);
  return result;
}

auto Compression::BitStream::Reader::read_raw_bytes(Count count)
    -> View::Bytes {
  // Align to byte boundary: discard fractional bits in the current byte.
  const Count fractional_bits = buffered_bits & 7;
  advance_bits(fractional_bits);

  // Rewind any whole bytes preloaded into buffer but not yet used.
  byte_position -= buffered_bits >> 3;
  buffer = 0;
  buffered_bits = 0;
  if (byte_position + count > data.get_size()) [[unlikely]] {
    invalid_stream = True;
    return View::Bytes();
  }

  auto result = data.slice(byte_position, count);
  byte_position += count;
  return result;
}

auto Compression::BitStream::Reader::peek_code(Count count) -> U32 {
  if (buffered_bits < count) {
    fill();
  }

  return U32(buffer) & ((U32(1) << count) - U32(1));
}
