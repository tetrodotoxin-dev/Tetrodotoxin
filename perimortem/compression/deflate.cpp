// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/compression/deflate.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/compression/bit_stream/reader.hpp"
#include "perimortem/compression/bit_stream/writer.hpp"
#include "perimortem/compression/huffman.hpp"
#include "perimortem/compression/lz77.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem;

// Distance codes zero through twenty nine use base distances and extra bits.
constexpr Static::Vector<U16, 30> distance_base = {{
  1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
  33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
  1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
}};

// Maps indexes to the correct code length alphabet ordering.
// The order puts the most commonly nonzero lengths first so the transmitted
// sequence can be truncated as soon as all trailing entries are zero.
constexpr Static::Vector<U8, 19> code_length_order = {{
  16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
}};

constexpr Count deflate_literal_len_count = 286;
constexpr Count deflate_distance_count = 30;

enum class BlockType : U8 {
  Stored = 0,
  FixedHuffman = 1,
  DynamicHuffman = 2,
};

enum class Encodings : U16 {
  Standard = 15,
  Repeat = 16,
  ShortZeros = 17,
  LongZeros = 18,
};

// A length or distance value mapped to its DEFLATE back reference encoding.
// Uses a base symbol index into the literal/length or distance alphabet plus
// an extra bits field that encodes the remainder.
class BackReferenceEncoding {
 public:
  constexpr BackReferenceEncoding()
      : symbol(0), extra_bits(0), extra_value(0) {}
  constexpr BackReferenceEncoding(
      Count symbol,
      U32 extra_value,
      Count extra_bits)
      : symbol(symbol), extra_bits(extra_bits), extra_value(extra_value) {}

  auto get_symbol() const -> Count { return symbol; }
  auto get_extra_value() const -> U32 { return extra_value; }
  auto get_extra_bits() const -> Count { return extra_bits; }

  // Length codes 257 through 285 use base lengths and extra bits.
  static constexpr Static::Vector<U16, 29> length_base = {{
    3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23,  27,
    31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258,
  }};

  static constexpr Static::Vector<U8, 29> length_extra_bits = {{
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
    2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0,
  }};

  // Precomputed O(1) encoding for every valid match length (3..258). Avoids the
  // backward linear scan over 29 entries for every encoded match.
  static constexpr auto build_length_encoding_table()
      -> Static::Vector<BackReferenceEncoding, 256> {
    constexpr Count length_symbol_offset = 257;
    Static::Vector<BackReferenceEncoding, 256> table;
    for (Count i = Compression::Lz77::min_match;
         i <= Compression::Lz77::max_match; i++) {
      Count code = 0;
      for (Count j = length_base.get_size() - 1; j > 0; j--) {
        if (i >= length_base[j]) {
          code = j;
          break;
        }
      }

      table[i - Compression::Lz77::min_match] = BackReferenceEncoding(
          length_symbol_offset + code, U32(i - length_base[code]),
          length_extra_bits[code]);
    }

    return table;
  }

 private:
  Count symbol;
  Count extra_bits;
  U32 extra_value;
};

constexpr Static::Vector<BackReferenceEncoding, 256> length_encoding_table =
    BackReferenceEncoding::build_length_encoding_table();

// A cached Lz77 output token which is either a literal byte or back reference.
// Caching the token stream lets deflate_dynamic run Lz77 once and build optimal
// Huffman codes from the cached frequencies before emitting.
//
// Match tokens store the distance encoding in advance so `emit_lz77_tokens`
// never calls encode_distance a second time.
class Token {
 public:
  Token() = default;
  Token(U8 literal_value)
      : length(0),
        extra_value(0),
        distance_symbol(0),
        extra_bits(0),
        literal(literal_value) {}
  Token(Count match_length, const BackReferenceEncoding& distance_encoding)
      : length(U16(match_length)),
        extra_value(U16(distance_encoding.get_extra_value())),
        distance_symbol(U8(distance_encoding.get_symbol())),
        extra_bits(U8(distance_encoding.get_extra_bits())),
        literal(0) {}

  auto is_match() const -> Bool {
    return length >= U16(Compression::Lz77::min_match);
  }

  auto get_length() const -> Count { return Count(length); }
  auto get_distance_symbol() const -> Count { return Count(distance_symbol); }
  auto get_extra_value() const -> U32 { return U32(extra_value); }
  auto get_extra_bits() const -> Count { return Count(extra_bits); }
  auto get_literal() const -> U8 { return literal; }

 private:
  U16 length;
  U16 extra_value;
  U8 distance_symbol;
  U8 extra_bits;
  U8 literal;
};

// A single entry in a run length encoded code length sequence.
class RleEntry {
 public:
  RleEntry() : extra_value(0), symbol(0), extra_bits(0) {}
  RleEntry(U8 symbol, U8 extra_bits, U32 extra_value)
      : extra_value(extra_value), symbol(symbol), extra_bits(extra_bits) {}

  auto get_symbol() const -> U8 { return symbol; }
  auto get_extra_bits() const -> U8 { return extra_bits; }
  auto get_extra_value() const -> U32 { return extra_value; }

 private:
  U32 extra_value;
  U8 symbol;
  U8 extra_bits;
};

// Validates all the header data in one go.
// The checks are light weight compared to the other checks so it should always
// be run, even on release builds.
constexpr auto validate_header(View::Bytes source) -> Bool {
  constexpr Count deflate_min_input_size = 7;
  constexpr U8 deflate_compression_method = 8;
  constexpr U32 deflate_fcheck_modulus = 31;
  if (source.get_size() < deflate_min_input_size) [[unlikely]] {
    Diagnostics::Log::error(
        "Compression: Input too short to be a valid deflate stream"_view);
    return False;
  }

  U8 deflate_cmf = source[0];
  U8 deflate_flag = source[1];
  if ((deflate_cmf & 0x0F) != deflate_compression_method) [[unlikely]] {
    Diagnostics::Log::error(
        "Compression: Unsupported compression method in deflate header"_view);
    return False;
  }

  if (((U32(deflate_cmf) * 256 + deflate_flag) % deflate_fcheck_modulus) != 0)
      [[unlikely]] {
    Diagnostics::Log::error(
        "Compression: Deflate header integrity check failed"_view);
    return False;
  }

  if (deflate_flag & 0x20) [[unlikely]] {
    Diagnostics::Log::error(
        "Compression: Preset dictionary is not supported"_view);
    return False;
  }

  return True;
}

// Writes the two header bytes.
constexpr auto write_header(Dynamic::Bytes& output) -> void {
  output.concat("\x78\x9c"_view);
}

// Adler 32 calculation for checksum operations.
constexpr auto calculate_checksum(View::Bytes data) -> U32 {
  constexpr U64 adler_modulus = 65521;
  constexpr Count batch_size = 5552;
  U64 s1 = 1;
  U64 s2 = 0;
  const U8* source_pointer = data.get_data();
  Count remaining = data.get_size();
  while (remaining >= batch_size) {
    for (Count i = 0; i < batch_size; i++) {
      s1 += source_pointer[i];
      s2 += s1;
    }

    s1 %= adler_modulus;
    s2 %= adler_modulus;
    source_pointer += batch_size;
    remaining -= batch_size;
  }

  for (Count i = 0; i < remaining; i++) {
    s1 += source_pointer[i];
    s2 += s1;
  }

  s1 %= adler_modulus;
  s2 %= adler_modulus;
  return U32((s2 << 16) | s1);
}

// O(1) distance encoding through bit width arithmetic.
constexpr auto encode_distance(Count d) -> BackReferenceEncoding {
  if (d <= 2) {
    return BackReferenceEncoding(d - 1, 0, 0);
  }

  const Count msb = Count(31 - __builtin_clzg(U32(d - 1), 32));
  const Count code = 2 * msb + (((d - 1) >> (msb - 1)) & 1);
  const Count extra_bits = msb - 1;
  return BackReferenceEncoding(code, U32(d - distance_base[code]), extra_bits);
}

constexpr auto inflate_symbols(
    const Compression::Huffman& literal_table,
    const Compression::Huffman& distance_table,
    Compression::BitStream::Reader& reader,
    Dynamic::Bytes& output) -> Bool {
  constexpr U16 end_of_block_symbol = 256;
  constexpr U16 length_code_start = 257;
  while (reader.is_valid()) {
    U16 symbol = literal_table.decode_symbol(reader);
    if (symbol == Compression::Huffman::invalid_symbol) [[unlikely]] {
      return False;
    }

    // Append regular symbols to the output
    if (symbol < end_of_block_symbol) {
      output.append(U8(symbol));
      continue;
    }

    // Complete the string if the end symbol is found.
    if (symbol == end_of_block_symbol) {
      return True;
    }

    // If the symbol is greater than end of block then it must be an extended
    // length code.
    Count length_code = symbol - length_code_start;
    if (length_code >= BackReferenceEncoding::length_base.get_size())
        [[unlikely]] {
      return False;
    }

    Count match_length =
        BackReferenceEncoding::length_base[length_code] +
        reader.read_code(BackReferenceEncoding::length_extra_bits[length_code]);

    U16 distance_symbol = distance_table.decode_symbol(reader);
    if (distance_symbol >= distance_base.get_size()) [[unlikely]] {
      return False;
    }

    constexpr Static::Vector<U8, 30> distance_extra_bits = {{
      0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
      6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
    }};

    // Caculate the offset for the match and if it's outside our currently
    // proccessed output then kill the inflate.
    Count match_offset = distance_base[distance_symbol] +
                         reader.read_code(distance_extra_bits[distance_symbol]);
    if (match_offset > output.get_size()) [[unlikely]] {
      return False;
    }

    // Set up the copy and write locations and make room to copy the earlier
    // part of the stream.
    Count write_start = output.get_size();
    Count copy_start = write_start - match_offset;
    output.resize(output.get_size() + match_length);
    auto bytes = output.get_access().get_data();

    // Ranges that do not overlap permit one bulk copy.
    if (match_offset >= match_length) {
      Data::copy(bytes + write_start, bytes + copy_start, match_length);
      continue;
    }

    // For overlapping repeated copies we have to copy the stride forward in
    // match_offset chunks.
    for (Count i = 0; i < match_length; i += match_offset) {
      Data::copy(
          bytes + write_start + i, bytes + copy_start,
          Math::min(match_offset, match_length - i));
    }
  }

  return False;
}

constexpr auto inflate_stored(
    Compression::BitStream::Reader& reader,
    Dynamic::Bytes& output) -> Bool {
  constexpr Count stored_block_header_size = 4;
  constexpr U16 stored_block_complement_check = 0xFFFF;

  const auto block_header = reader.read_raw_bytes(stored_block_header_size);
  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  U16 block_length =
      Data::ensure_endian<Data::ByteOrder::Little, Data::ByteOrder::Native>(
          *Data::cast<U16>(block_header.get_data()));
  U16 block_length_complement =
      Data::ensure_endian<Data::ByteOrder::Little, Data::ByteOrder::Native>(
          *Data::cast<U16>(block_header.get_data() + 2));
  if (U16(block_length ^ block_length_complement) !=
      stored_block_complement_check) [[unlikely]] {
    return False;
  }

  auto block_data = reader.read_raw_bytes(block_length);
  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  output.concat(block_data);
  return True;
}

constexpr auto inflate_fixed(
    Compression::BitStream::Reader& reader,
    Dynamic::Bytes& output) -> Bool {
  constexpr Compression::Huffman literal_table =
      Compression::Huffman::make_fixed_literal();
  constexpr Compression::Huffman distance_table =
      Compression::Huffman::make_fixed_distance();
  return inflate_symbols(literal_table, distance_table, reader, output);
}

constexpr auto inflate_dynamic(
    Compression::BitStream::Reader& reader,
    Dynamic::Bytes& output) -> Bool {
  Count literal_code_count = reader.read_code(5) + 257;
  Count distance_code_count = reader.read_code(5) + 1;
  Count huffman_code_count = reader.read_code(4) + 4;
  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  Static::Vector<U8, code_length_order.get_size()> huffman_lengths;
  for (Count i = 0; i < huffman_code_count; i++) {
    huffman_lengths[code_length_order[i]] = U8(reader.read_code(3));
  }

  const auto dynamic_table = Compression::Huffman(huffman_lengths);
  Count total_codes = literal_code_count + distance_code_count;
  Static::Vector<U8, Compression::Huffman::max_symbol_count>
      actual_literal_len_lengths;

  Count decode_index = 0;
  while (decode_index < total_codes && reader.is_valid()) {
    U16 symbol = dynamic_table.decode_symbol(reader);

    U8 value = 0;
    Count element_count = Count(-1);
    switch (symbol) {
    case 0 ... U16(Encodings::Standard):
      value = U8(symbol);
      element_count = 1;
      break;
    case U16(Encodings::Repeat):
      value =
          decode_index > 0 ? actual_literal_len_lengths[decode_index - 1] : 0;
      element_count = reader.read_code(2) + 3;
      break;
    case U16(Encodings::ShortZeros):
      value = 0;
      element_count = reader.read_code(3) + 3;
      break;
    case U16(Encodings::LongZeros):
      value = 0;
      element_count = reader.read_code(7) + 11;
      break;
    }

    if (decode_index + element_count > total_codes) {
      Diagnostics::Log::error(
          "Compression: dynamic block code-length sequence overruns total code count"_view);
      return False;
    }

    Data::set(
        actual_literal_len_lengths.get_data() + decode_index, value,
        element_count);
    decode_index += element_count;
  }

  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  const auto literal_symbols =
      actual_literal_len_lengths.get_view().slice(0, literal_code_count);
  const auto distance_symbols =
      actual_literal_len_lengths.get_view().slice(literal_code_count);

  // Reject overcommitted prefix codes. A sum above the maximum means at least
  // two codes share a prefix, making decoding ambiguous and the stream invalid
  // per RFC 1951. This strict check ensures Perimortem inflate agrees with
  // every conformant DEFLATE decoder rather than silently tolerating bad
  // tables.
  auto is_valid_kraft = [](View::Vector<U8> lengths) -> Bool {
    const auto* length_data = lengths.get_data();
    Count max_length = 0;
    for (Count i = 0; i < lengths.get_size(); i++) {
      if (Count(length_data[i]) > max_length) {
        max_length = Count(length_data[i]);
      }
    }

    if (max_length == 0) {
      return True;
    }

    Count kraft = 0;
    for (Count i = 0; i < lengths.get_size(); i++) {
      if (length_data[i] > 0) {
        kraft += Count(1) << (max_length - Count(length_data[i]));
      }
    }

    return kraft <= (Count(1) << max_length);
  };
  if (!is_valid_kraft(literal_symbols)) [[unlikely]] {
    Diagnostics::Log::error(
        "Compression: literal/length Huffman table violates Kraft inequality"_view);
    return False;
  }

  if (!is_valid_kraft(distance_symbols)) [[unlikely]] {
    Diagnostics::Log::error(
        "Compression: distance Huffman table violates Kraft inequality"_view);
    return False;
  }

  return inflate_symbols(
      Compression::Huffman(literal_symbols),
      Compression::Huffman(distance_symbols), reader, output);
}

constexpr auto write_checksum(Dynamic::Bytes& output, View::Bytes source)
    -> void {
  U32 checksum = calculate_checksum(source);

  output.resize(output.get_size() + sizeof(U32));
  Data::write<Data::ByteOrder::Big>(
      Data::cast<U32>(
          output.get_access().get_data() + output.get_size() - sizeof(U32)),
      checksum);
}

constexpr auto test_checksum(View::Bytes stream, View::Bytes source) -> Bool {
  U32 stream_checksum = calculate_checksum(stream);

  U32 checksum =
      Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
          *Data::cast<U32>(
              source.get_data() + source.get_size() - sizeof(U32)));
  if (stream_checksum != checksum) [[unlikely]] {
    Diagnostics::Log::Message<128> error_message(
        Diagnostics::Log::Level::Error);
    error_message << "Compression: Adler-32 checksum mismatch. checksum="_view
                  << checksum << " stream_checksum="_view << stream_checksum;
    return False;
  }

  return True;
}

constexpr auto write_lz77_block(
    View::Bytes source,
    Count search_depth,
    Compression::Lz77& lz77,
    Compression::BitStream::Writer& writer,
    const Compression::Huffman& literal_table,
    const Compression::Huffman& distance_table) -> void {
  Count position = 0;
  while (position < source.get_size()) {
    auto match = lz77.find_match_and_insert(source, position, search_depth);
    if (match.get_length() >= Compression::Lz77::min_match) {
      const auto& length_encoding = length_encoding_table
          [match.get_length() - Compression::Lz77::min_match];
      auto literal_code =
          literal_table.encode_symbol(length_encoding.get_symbol());
      writer.write_code(literal_code.get_code(), literal_code.get_length());
      writer.write_bits(
          length_encoding.get_extra_value(), length_encoding.get_extra_bits());

      const auto distance_encoding = encode_distance(match.get_distance());
      auto distance_code =
          distance_table.encode_symbol(distance_encoding.get_symbol());
      writer.write_code(distance_code.get_code(), distance_code.get_length());
      writer.write_bits(
          distance_encoding.get_extra_value(),
          distance_encoding.get_extra_bits());
      position += match.get_length();
    } else {
      auto literal_code = literal_table.encode_symbol(Count(source[position]));
      writer.write_code(literal_code.get_code(), literal_code.get_length());
      position++;
    }
  }
}

constexpr auto write_deflate_footer(
    Compression::BitStream::Writer& writer,
    const Compression::Huffman& literal_table,
    Dynamic::Bytes& output,
    View::Bytes source) -> void {
  constexpr Count end_of_block = 256;
  auto eob_code = literal_table.encode_symbol(end_of_block);

  writer.write_code(eob_code.get_code(), eob_code.get_length());
  writer.flush();
  write_checksum(output, source);
}

constexpr auto rle_encode_code_lengths(
    View::Vector<U8> literal_len_lengths,
    View::Vector<U8> distance_lengths,
    Static::Vector<RleEntry, 320>& output) -> Count {
  const Count literal_length_count = literal_len_lengths.get_size();
  const auto* literal_data = literal_len_lengths.get_data();
  const auto* distance_data = distance_lengths.get_data();
  auto lengths_at = [&](Count index) -> U8 {
    return index < literal_length_count
               ? literal_data[index]
               : distance_data[index - literal_length_count];
  };

  const Count total = literal_length_count + distance_lengths.get_size();
  Count count = 0;
  Count position = 0;
  while (position < total) {
    U8 length = lengths_at(position);
    if (length == 0) {
      Count run = 1;
      while (position + run < total && lengths_at(position + run) == 0 &&
             run < 138) {
        run++;
      }

      if (run >= 11) {
        output[count++] = RleEntry(U8(18), U8(7), U32(run - 11));
      } else if (run >= 3) {
        output[count++] = RleEntry(U8(17), U8(3), U32(run - 3));
      } else {
        for (Count r = 0; r < run; r++) {
          output[count++] = RleEntry();
        }
      }

      position += run;
    } else {
      output[count++] = RleEntry(length, U8(0), U32(0));
      position++;
      Count run = 0;
      while (position + run < total && lengths_at(position + run) == length &&
             run < 6) {
        run++;
      }

      if (run >= 3) {
        output[count++] = RleEntry(U8(16), U8(2), U32(run - 3));
        position += run;
      }
    }
  }

  return count;
}

constexpr auto write_dynamic_block_header(
    Compression::BitStream::Writer& writer,
    View::Vector<U8> literal_len_lengths,
    Count literal_index,
    View::Vector<U8> distance_lengths,
    Count distance_index) -> void {
  Static::Vector<RleEntry, 320> rle;
  Count rle_count = rle_encode_code_lengths(
      View::Vector<U8>(literal_len_lengths.get_data(), literal_index + 257),
      View::Vector<U8>(distance_lengths.get_data(), distance_index + 1), rle);

  Static::Vector<U32, code_length_order.get_size()> meta_freq;
  for (Count i = 0; i < code_length_order.get_size(); i++) {
    meta_freq[i] = 0;
  }

  for (Count i = 0; i < rle_count; i++) {
    meta_freq[rle[i].get_symbol()]++;
  }

  Static::Vector<U8, code_length_order.get_size()> meta_lengths;
  Compression::Huffman::compute_lengths(
      meta_freq.get_view(), meta_lengths.get_access());
  const Compression::Huffman meta_table(meta_lengths.get_view());

  // Trim trailing zero length entries from the meta table.
  // The minimum allowed is 4, so we stop at index 3.
  Count code_length = code_length_order.get_size() - 1;
  while (code_length > 3 && meta_lengths[code_length_order[code_length]] == 0) {
    code_length--;
  }

  writer.write_bits(U32(literal_index), 5);
  writer.write_bits(U32(distance_index), 5);
  writer.write_bits(U32(code_length - 3), 4);
  for (Count i = 0; i <= code_length; i++) {
    writer.write_bits(meta_lengths[code_length_order[i]], 3);
  }

  for (Count i = 0; i < rle_count; i++) {
    auto meta_code = meta_table.encode_symbol(rle[i].get_symbol());
    writer.write_code(meta_code.get_code(), meta_code.get_length());
    writer.write_bits(rle[i].get_extra_value(), rle[i].get_extra_bits());
  }
}

constexpr auto deflate_stored(View::Bytes source) -> Dynamic::Bytes {
  constexpr Count max_stored_block_size = 1 << 16;
  constexpr Count deflate_header_size = 2;
  constexpr Count stored_block_header_size = 5;
  const Count block_count = source.get_size() / max_stored_block_size + 1;

  const Count output_size = deflate_header_size +
                            block_count * stored_block_header_size +
                            source.get_size() + sizeof(U32);

  Dynamic::Bytes output;
  output.ensure_capacity(output_size);
  write_header(output);
  for (Count i = 0; i <= source.get_size();) {
    const Count block_size =
        Math::min(source.get_size() - i, max_stored_block_size);
    const U8 is_final_block =
        (i + block_size >= source.get_size()) ? 0x01 : 0x00;

    output.append(U8(is_final_block));
    output.append(U8(block_size & 0xFF));
    output.append(U8((block_size >> 8) & 0xFF));
    output.append(U8((~block_size) & 0xFF));
    output.append(U8(((~block_size) >> 8) & 0xFF));
    if (block_size > 0) {
      output.concat(source.slice(i, block_size));
    }

    if (is_final_block) {
      break;
    }

    i += block_size;
  }

  write_checksum(output, source);
  return output;
}

constexpr auto deflate_fixed(View::Bytes source, Count search_depth)
    -> Dynamic::Bytes {
  constexpr Compression::Huffman literal_table =
      Compression::Huffman::make_fixed_literal();
  constexpr Compression::Huffman distance_table =
      Compression::Huffman::make_fixed_distance();

  Dynamic::Bytes output;
  output.ensure_capacity(source.get_size() + 128);
  write_header(output);

  Compression::BitStream::Writer writer(output);
  writer.write_bits(0x01, 1);
  writer.write_bits(U32(BlockType::FixedHuffman), 2);
  if (source.get_size() > 0) {
    Compression::Lz77 lz77;
    write_lz77_block(
        source, search_depth, lz77, writer, literal_table, distance_table);
  }

  write_deflate_footer(writer, literal_table, output, source);
  return output;
}

constexpr auto collect_lz77_tokens(
    View::Bytes source,
    Count search_depth,
    Compression::Lz77& lz77,
    Dynamic::Vector<Token>& tokens,
    Static::Vector<U32, deflate_literal_len_count>& literal_len_frequencies,
    Static::Vector<U32, deflate_distance_count>& distance_frequencies) -> void {
  // Matches at least this long are emitted immediately without looking ahead.
  // the extra chain search is wasted work when the current match is already
  // good. Same heuristic as zlib level 6.
  constexpr Count lazy_match_threshold = 32;

  Count position = 0;
  while (position < source.get_size()) {
    auto match = lz77.find_match_and_insert(source, position, search_depth);

    // Lazy matching: for short matches, check whether position+1 yields a
    // longer one. If so, emit a literal at position and take the better match
    // from position+1.
    if (match.get_length() >= Compression::Lz77::min_match &&
        match.get_length() < lazy_match_threshold &&
        position + 1 < source.get_size()) {
      auto next_match =
          lz77.find_match_and_insert(source, position + 1, search_depth);
      if (next_match.get_length() > match.get_length()) {
        tokens.insert(Token(U8(source[position])));
        literal_len_frequencies[Count(source[position])]++;
        position++;
        match = next_match;
      }
    }

    if (match.get_length() >= Compression::Lz77::min_match) {
      const auto distance_encoding = encode_distance(match.get_distance());
      tokens.insert(Token(match.get_length(), distance_encoding));
      literal_len_frequencies[length_encoding_table
                                  [match.get_length() -
                                   Compression::Lz77::min_match]
                                      .get_symbol()]++;
      distance_frequencies[distance_encoding.get_symbol()]++;

      // Insert the last position of the match so the next block has a
      // previous distance candidate. Without this, long matches skip
      // intermediate positions and the next block's nearest chain entry is
      // match_length
      // bytes back, which costs extra bits on the distance code for every
      // subsequent match in a run (e.g., gradient rows after Up filtering).
      const Count last_position = position + match.get_length() - 1;
      if (last_position + Compression::Lz77::min_match < source.get_size()) {
        lz77.insert(source, last_position);
      }

      position += match.get_length();
    } else {
      tokens.insert(Token(U8(source[position])));
      literal_len_frequencies[Count(source[position])]++;
      position++;
    }
  }
}

constexpr auto emit_lz77_tokens(
    View::Vector<Token> tokens,
    Compression::BitStream::Writer& writer,
    const Compression::Huffman& literal_table,
    const Compression::Huffman& distance_table) -> void {
  const auto* token_data = tokens.get_data();
  for (Count i = 0; i < tokens.get_size(); i++) {
    const Token& token = token_data[i];
    if (token.is_match()) {
      const auto& length_encoding = length_encoding_table
          [token.get_length() - Compression::Lz77::min_match];
      auto literal_code =
          literal_table.encode_symbol(length_encoding.get_symbol());
      writer.write_code(literal_code.get_code(), literal_code.get_length());
      writer.write_bits(
          length_encoding.get_extra_value(), length_encoding.get_extra_bits());

      auto distance_code =
          distance_table.encode_symbol(token.get_distance_symbol());
      writer.write_code(distance_code.get_code(), distance_code.get_length());
      writer.write_bits(token.get_extra_value(), token.get_extra_bits());
    } else {
      auto literal_code =
          literal_table.encode_symbol(Count(token.get_literal()));
      writer.write_code(literal_code.get_code(), literal_code.get_length());
    }
  }
}

constexpr auto deflate_dynamic(View::Bytes source, Count search_depth)
    -> Dynamic::Bytes {
  if (source.is_empty()) {
    return deflate_fixed(source, search_depth);
  }

  // Create lz77 and start populating frequency tables.
  Compression::Lz77 lz77;
  Static::Vector<U32, deflate_distance_count> distance_frequencies;
  Static::Vector<U32, deflate_literal_len_count> literal_len_frequencies;
  literal_len_frequencies[256] = 1;

  // Single lz77 pass to collect tokens and count frequencies simultaneously.
  // Worst case token count is one per min_match bytes but in practice back
  // references make it much smaller so this is a generous upper bound.
  Dynamic::Vector<Token> tokens(
      source.get_size() / Compression::Lz77::min_match + 64);
  collect_lz77_tokens(
      source, search_depth, lz77, tokens, literal_len_frequencies,
      distance_frequencies);

  Static::Vector<U8, deflate_literal_len_count> literal_len_lengths;
  Static::Vector<U8, deflate_distance_count> distance_lengths;
  Compression::Huffman::compute_lengths(
      literal_len_frequencies.get_view(), literal_len_lengths.get_access());
  Compression::Huffman::compute_lengths(
      distance_frequencies.get_view(), distance_lengths.get_access());
  if (literal_len_lengths[256] == 0) {
    literal_len_lengths[256] = 1;
  }

  Bool distance_found = False;
  for (Count i = 0; i < deflate_distance_count; i++) {
    if (distance_lengths[i] > 0) {
      distance_found = True;
      break;
    }
  }

  if (!distance_found) {
    distance_lengths[0] = 1;
  }

  // Create huffman tables for translation.
  const Compression::Huffman literal_len_table(literal_len_lengths.get_view());
  const Compression::Huffman distance_table(distance_lengths.get_view());

  Count literal_index = 0;
  for (Count i = deflate_literal_len_count - 1; i > 256; i--) {
    if (literal_len_lengths[i] > 0) {
      literal_index = i - 256;
      break;
    }
  }

  Count distance_index = 0;
  for (Count i = deflate_distance_count - 1; i > 0; i--) {
    if (distance_lengths[i] > 0) {
      distance_index = i;
      break;
    }
  }

  Dynamic::Bytes output;
  output.ensure_capacity(source.get_size() / 2 + 1024);
  write_header(output);

  Compression::BitStream::Writer writer(output);
  writer.write_bits(0x01, 1);
  writer.write_bits(U32(BlockType::DynamicHuffman), 2);
  write_dynamic_block_header(
      writer, literal_len_lengths.get_view(), literal_index,
      distance_lengths.get_view(), distance_index);

  // Emit the cached token stream
  emit_lz77_tokens(
      tokens.get_view(), writer, literal_len_table, distance_table);

  write_deflate_footer(writer, literal_len_table, output, source);
  return output;
}

auto Compression::Deflate::inflate(
    const View::Bytes source,
    Count capacity_hint) -> Dynamic::Bytes {
  if (!validate_header(source)) {
    return Dynamic::Bytes();
  }

  Compression::BitStream::Reader reader{source.slice(2, source.get_size() - 6)};
  Dynamic::Bytes output;
  output.ensure_capacity(
      capacity_hint > 0 ? capacity_hint : source.get_size() * 4);

  Bool final_block = False;
  while (reader.is_valid() && !final_block) {
    final_block = reader.read_code(1);

    auto block_type = BlockType(reader.read_code(2));
    switch (block_type) {
    case BlockType::Stored:
      if (!inflate_stored(reader, output)) {
        Diagnostics::Log::error(
            "Compression: stored block decompression failed"_view);
        return Dynamic::Bytes();
      }

      break;
    case BlockType::FixedHuffman:
      if (!inflate_fixed(reader, output)) {
        Diagnostics::Log::error(
            "Compression: fixed Huffman block decompression failed"_view);
        return Dynamic::Bytes();
      }

      break;
    case BlockType::DynamicHuffman:
      if (!inflate_dynamic(reader, output)) {
        Diagnostics::Log::error(
            "Compression: dynamic Huffman block decompression failed"_view);
        return Dynamic::Bytes();
      }

      break;
    default:
      Diagnostics::Log::error(
          "Compression: unknown block type in deflate stream"_view);
      return Dynamic::Bytes();
    }
  }

  if (!reader.is_valid()) [[unlikely]] {
    return Dynamic::Bytes();
  }

#if PERI_DEBUG
  if (!test_checksum(output, source)) {
    return Dynamic::Bytes();
  }
#endif

  return output;
}

auto Compression::Deflate::deflate(View::Bytes source, Level level)
    -> Dynamic::Bytes {
  constexpr Count default_search_depth = 8;
  constexpr Count best_search_depth = 128;
  switch (level) {
  case Level::None:
    return deflate_stored(source);
  case Level::Default:
    return deflate_dynamic(source, default_search_depth);
  case Level::Best:
    return deflate_dynamic(source, best_search_depth);
  default: {
    Diagnostics::Log::Message<128> warning_message(
        Diagnostics::Log::Level::Warning);
    warning_message << "Compression: unknown compression level "_view
                    << U8(level)
                    << " requested, defaulting to Level::Default (2)"_view;
    return deflate_dynamic(source, default_search_depth);
  }
  }
}
