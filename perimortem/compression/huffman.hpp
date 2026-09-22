// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/compression/bit_stream/reader.hpp"

namespace Perimortem::Compression {

// Builds the canonical Huffman codes used to encode and decode DEFLATE symbol
// streams. One set of code lengths produces both directions, which prevents
// the writer and reader from drifting into separate interpretations of the
// same tree. All lookup storage is held directly by the value.
class Huffman {
 public:
  class Code {
   public:
    constexpr Code() {}
    constexpr Code(U32 code, Count length) : length(length), code(code) {}

    constexpr auto get_code() const -> U32 { return code; }
    constexpr auto get_length() const -> Count { return length; }

   private:
    Count length = 0;
    U32 code = 0;
  };

  static constexpr U16 invalid_symbol = 0xFFFF;
  static constexpr Count max_code_bits = 15;
  static constexpr Count max_symbol_count = 320;

  constexpr Huffman(Core::View::Vector<U8> code_lengths) {
    const auto* lengths = code_lengths.get_data();
    Core::Static::Vector<Count, max_code_bits + 2> bit_length_count;
    for (Count symbol = 0; symbol < code_lengths.get_size(); symbol++) {
      if (lengths[symbol] > 0) {
        bit_length_count[lengths[symbol]]++;
        if (lengths[symbol] > max_bits) {
          max_bits = lengths[symbol];
        }
      }
    }

    U32 canonical_code = 0;
    for (Count i = 1; i <= max_bits; i++) {
      base_code[i] = canonical_code;
      canonical_code = (canonical_code + bit_length_count[i]) << 1;
    }

    Count symbol_index = 0;
    for (Count i = 1; i <= max_bits + 1; i++) {
      base_index[i] = symbol_index;
      symbol_index += bit_length_count[i];
    }

    Core::Static::Vector<Count, max_code_bits + 2> fill_offset;
    for (Count i = 1; i <= max_bits; i++) {
      fill_offset[i] = base_index[i];
    }

    for (Count i = 0; i < code_lengths.get_size(); i++) {
      Count length = lengths[i];
      if (length > 0) {
        Count position = fill_offset[length]++;
        symbol_map[position] = U16(i);
        // Store codes in bit reversed form so the writer can place them
        // directly into the accumulator without a per symbol reversal.
        U32 canonical = base_code[length] + U32(position - base_index[length]);
        U32 reversed = 0;
        for (Count bit = 0; bit < length; bit++) {
          reversed = (reversed << 1) | (canonical & 1);
          canonical >>= 1;
        }

        encode_codes[i] = reversed;
        encode_lengths[i] = U8(length);
      }
    }

    // Populate the 9 bit fast decode table. The encode table already holds the
    // reversed bits, so each entry can use the stored stream value directly.
    for (Count symbol = 0; symbol < code_lengths.get_size(); symbol++) {
      Count length = lengths[symbol];
      if (length == 0 || length > fast_bits) {
        continue;
      }

      U32 stream_bits = encode_codes[symbol];
      Count extensions = fast_table_size >> length;
      for (Count k = 0; k < extensions; k++) {
        fast_table[stream_bits | (k << length)] =
            FastEntry{U16(symbol), U8(length)};
      }
    }
  }

  // Simple generator for creating all the fixed literal lengths.
  static constexpr auto make_fixed_literal() -> Huffman {
    Core::Static::Vector<U8, 288> code_lengths([](Count i) -> U8 {
      switch (i) {
      case 144 ... 255:
        return 9;
      case 256 ... 279:
        return 7;
      default:
        return 8;
      }
    });
    return Huffman(code_lengths.get_view());
  }

  static auto compute_lengths(
      Core::View::Vector<U32> frequencies,
      Core::Access::Vector<U8> lengths) -> void;

  static constexpr auto make_fixed_distance() -> Huffman {
    Core::Static::Vector<U8, 30> code_lengths([](Count i) -> U8 { return 5; });
    return Huffman(code_lengths.get_view());
  }

  constexpr auto decode_symbol(BitStream::Reader& reader) const -> U16 {
    // Fast path: one 9 bit peek + table lookup covers codes up to 9 bits long.
    const auto fast_key = reader.peek_code(fast_bits);
    const FastEntry& fast_entry = fast_table[fast_key];
    if (fast_entry.length > 0) {
      reader.advance_bits(fast_entry.length);
      return fast_entry.symbol;
    }

    // Slow path: bit by bit accumulation for codes longer than 9 bits.
    U32 accumulated_code = 0;
    for (Count i = 1; i <= max_bits; i++) {
      accumulated_code = (accumulated_code << 1) | reader.read_bit().value;
      const auto code_index = base_index[i];
      const auto base = base_code[i];
      Count symbol_count = base_index[i + 1] - code_index;
      if (symbol_count > 0 && accumulated_code >= base &&
          (accumulated_code - base) < symbol_count) {
        return symbol_map[code_index + (accumulated_code - base)];
      }
    }

    reader.invalidate();
    return invalid_symbol;
  }

  constexpr auto encode_symbol(Count symbol) const -> Code {
    if (symbol >= max_symbol_count || encode_lengths[symbol] == 0) {
      return Code();
    }

    return Code(encode_codes[symbol], encode_lengths[symbol]);
  }

 private:
  struct FastEntry {
    U16 symbol = invalid_symbol;
    U8 length = 0;
  };

  static constexpr Count fast_bits = 9;
  static constexpr Count fast_table_size = 1 << fast_bits;

  Core::Static::Vector<FastEntry, fast_table_size> fast_table;
  Core::Static::Vector<U32, max_symbol_count> encode_codes;
  Core::Static::Vector<U16, max_symbol_count> symbol_map;
  Core::Static::Vector<Count, max_code_bits + 2> base_index;
  Core::Static::Vector<U32, max_code_bits + 2> base_code;
  Core::Static::Vector<U8, max_symbol_count> encode_lengths;
  Count max_bits = 0;
};

}  // namespace Perimortem::Compression
