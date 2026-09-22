// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/writer/binary.hpp"

namespace Ttx::Data::Encoding {

// Fields in Struct, Element and Callable can share a U64 chunk or cross into
// the next one. Block assembles those fields before writing each chunk so all
// three formats use the same packing rules. The largest profile contains
// fifteen U32 chunks, which fit in eight U64 scratch chunks. Updating those
// scratch chunks required less memory traffic and measured faster than merging
// each field directly into the destination.
//
// A reader needs only the chunks containing the requested field. extract()
// locates them by their bit coordinates in the complete buffer. Canonical
// padding makes every U64 load complete, even across descriptor boundaries.
// Reading and writing both support unaligned storage in little endian order.
class Block {
 public:
  constexpr Block() = default;

  // Fields are disjoint and this assembler starts at zero. OR therefore
  // preserves earlier fields, including when this value crosses into the next
  // chunk. Supported format fields start below bit 384, so any spill fits
  // this scratch array. Wider fields retain zero high bits beyond Count.
  constexpr auto insert(Count value, Count first) -> void {
    const Count chunk_64 = first >> 6;
    const Count shift = first & 0x3f;
    chunks_64[chunk_64] |= value << shift;
    if (shift) {
      chunks_64[chunk_64 + 1] |= value >> (64 - shift);
    }
  }

  // Bytes contains the complete admitted Representation, and first is an
  // absolute bit coordinate within it. A descriptor can start halfway through
  // a U64 chunk, so loads must be aligned relative to the buffer's start.
  // The field needs its containing chunk and, if it crosses that boundary,
  // the following chunk. Canonical padding makes both loads complete.
  //
  // The result contains the field's low bits that fit in Count. Numeric fields
  // have zero high bits. Wider markers such as void require several slices to
  // inspect all their bits.
  static constexpr auto extract(
      Perimortem::Core::View::Bytes bytes,
      Count first,
      Count width) -> Count {
    const auto chunk = [&](Count offset) -> U64 {
      // Clang permits builtin copies between byte arrays during constant
      // evaluation. bit_cast can then read that array as a U64 without casting
      // a pointer. The same code reduces to an unaligned U64 load at runtime.
      U8 chunk_bytes[sizeof(U64)];
      __builtin_memcpy(chunk_bytes, bytes.get_data() + offset, sizeof(U64));
      return Perimortem::Core::Data::ensure_endian<
          Perimortem::Core::Data::ByteOrder::Little,
          Perimortem::Core::Data::ByteOrder::Native>(
          __builtin_bit_cast(U64, chunk_bytes));
    };
    const Count offset = (first >> 6) << 3;
    const Count shift = first & 0x3f;
    Count value = chunk(offset) >> shift;
    if (shift && width > 64 - shift) {
      value |= chunk(offset + 8) << (64 - shift);
    }

    return width >= 64 ? value : value & ((Count(1) << width) - 1);
  }

  // Callable's void selector uses all ones, including beyond native Count.
  // Fill it in pieces that fit Count so no operation shifts by its own width.
  constexpr auto fill(Count first, Count width) -> void {
    while (width) {
      const Count bits = width < 64 ? width : 64;
      insert(bits == 64 ? Count(-1) : (Count(1) << bits) - 1, first);
      first += bits;
      width -= bits;
    }
  }

  // F counts U32 words, so a block of odd depth ends with four bytes before
  // the next block. Rounding each block would change its neighbours' indices.
  // Only Compiler's final padding rounds the complete buffer to U64 chunks.
  constexpr auto write(
      Perimortem::Core::Writer::Binary<
          Perimortem::Core::Data::ByteOrder::Little>& writer,
      U8 depth) const -> void {
    for (Count i = 0; i < (depth >> 1); ++i) {
      writer << chunks_64[i];
    }

    if (depth & 1) {
      writer << U32(chunks_64[depth >> 1]);
    }
  }

 private:
  U64 chunks_64[8] = {};
};

}  // namespace Ttx::Data::Encoding
