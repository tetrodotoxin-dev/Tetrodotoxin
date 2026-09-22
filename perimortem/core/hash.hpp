// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Core {

class Hash {
  // Since memcpy isn't constexpr but is guaranteed to give us the optimum
  // runtime logic but Clang isn't smart enough to do the substitution for the
  // constexpr version bit shift verion to a std::memcpy, we can wrap the logic
  // to ensure the slower logic is never executed at runtime.
  template <Count chunks>
  static constexpr auto memcpy_64(U64 (&result)[chunks], const U8* data)
      -> void {
    if consteval {
      for (Count chunk = 0; chunk < chunks; chunk++) {
        result[chunk] = 0;
        result[chunk] |= static_cast<U64>(data[7]) << 56;
        result[chunk] |= static_cast<U64>(data[6]) << 48;
        result[chunk] |= static_cast<U64>(data[5]) << 40;
        result[chunk] |= static_cast<U64>(data[4]) << 32;
        result[chunk] |= static_cast<U64>(data[3]) << 24;
        result[chunk] |= static_cast<U64>(data[2]) << 16;
        result[chunk] |= static_cast<U64>(data[1]) << 8;
        result[chunk] |= static_cast<U64>(data[0]) << 0;

        data += sizeof(U64);
      }
    } else {
      memcpy(&result, data, sizeof(U64) * chunks);
    }
  }

  static constexpr auto hash_numeric(U64 numeric) {
    U64 result = numeric * const_values[0];
    result ^= (result >> 33) * const_values[4];
    return result;
  }

 public:
  constexpr Hash(Core::View::Bytes bytes) {
    const U8* data = bytes.get_data();

    // Fast path for keys less than 8 bytes, but not the full 16.
    // This enables the underflow case when dealing with the remainder to drop a
    // redundant check while also minimizing the load on the icache providing
    // better results on the training data.
    // The results may be caused by overfitting but the fact it results in less
    // code to manage makes it the preferred method.
    U64 small_hash_value = 0;
    switch (bytes.get_size()) {
    case 7:
      small_hash_value |= static_cast<U64>(data[6]) << 48;
    case 6:
      small_hash_value |= static_cast<U64>(data[5]) << 40;
    case 5:
      small_hash_value |= static_cast<U64>(data[4]) << 32;
    case 4:
      small_hash_value |= static_cast<U64>(data[3]) << 24;
    case 3:
      small_hash_value |= static_cast<U64>(data[2]) << 16;
    case 2:
      small_hash_value |= static_cast<U64>(data[1]) << 8;
    case 1: {
      small_hash_value |= static_cast<U64>(data[0]);

      small_hash_value = const_values[0] ^ (small_hash_value + const_values[3]);
      small_hash_value *= const_values[2];

      // Avalanche the small hash.
      value = small_hash_value ^ (bytes.get_size() * const_values[0]);
      value ^= value >> 27;
      value *= const_values[2];
      value ^= value >> 33;
      value *= const_values[4];
      value ^= value >> 33;
      return;
    }

    // Empty input uses one fixed value so callers receive a stable hash without
    // reading absent storage.
    case 0:
      value = const_values[4];
      return;
    }

    // Prep blocks to prime cascade.
    U64 result[block_stride];
    const Count block_count = bytes.get_size() / sizeof(U64);
    for (Count i = 0; i < block_stride; i++) {
      result[i] = const_values[i];
    }

    // Bulk process chunks to improve throughput metrics.
    // Core loop fits in two decode blocks on Zen3+
    // (https://godbolt.org/z/KzK6dPM4G).
    // The biggest bubble in the pipeline is the imul which gives us better
    // distribution on longer keys but could possibly be dropped for a simpler
    // cascade.
    [[clang::code_align(32)]]
    for (Count processed = 0; processed + block_stride <= block_count;
         processed += block_stride) {
      // Let the compiler deal with possibly unaligned data for both x86_64 and
      // ARM if we ever decide to support ARM down the road.
      //
      // Using SSE 128 is slightly slower anyway when we are only processing 2
      // block chunks.
      U64 block[block_stride];
      memcpy_64(block, data);
      data += sizeof(U64) * block_stride;
      for (Count k = 0; k < block_stride; k++) {
        result[k] ^= block[k] + const_values[k + 3];
        result[k] *= const_values[k + 1];
      }
    }

    // Since we've already dealt with small payloads we can handle the remaining
    // bytes with three possible cases:
    Count left_over = bytes.get_size() & (sizeof(U64) * block_stride - 1);
    switch (left_over) {
    // Case 1: We need to under read the higher 64bits and shift, then we can
    //         fallthrough to case 2.
    case 15:
    case 14:
    case 13:
    case 12:
    case 11:
    case 10:
    case 9: {
      // Under read the buffer so we ensure we don't read outside of bounds.
      U64 remainder_tail[1];
      memcpy_64(remainder_tail, data + left_over - sizeof(U64));

      // Shift out the extra bites
      remainder_tail[0] >>= (sizeof(U64) * 2 - left_over) * 8;

      result[1] ^= remainder_tail[0] + const_values[1];
      result[1] *= const_values[2];

      [[fallthrough]];
    }

    // Case 2: We have exactly 8 bytes left so read them in a single read.
    case 8: {
      // Read the exact 8 bytes left over.
      U64 remainder[1];
      memcpy_64(remainder, data);

      result[0] ^= remainder[0] + const_values[3];
      result[0] *= const_values[2];
      break;
    }

    // Case 4: The buffer was an exact multiple of 16 bytes.
    case 7:
    case 6:
    case 5:
    case 4:
    case 3:
    case 2:
    case 1: {
      // Read the exact 8 bytes left over.
      U64 remainder[1];
      memcpy_64(remainder, data + left_over - sizeof(U64));

      // Shift out the extra bites
      remainder[0] >>= (sizeof(U64) - left_over) * 8;

      result[0] ^= remainder[0] + const_values[3];
      result[0] *= const_values[2];
      break;
    }

    // Case 4: The buffer was an exact multiple of 16 bytes.
    case 0:
      break;
    }

    // Perform final avalanche
    U64 final_result = result[0];
    for (Count i = 0; i < block_stride; i++) {
      result[i] ^= bytes.get_size() * const_values[0];
      result[i] ^= result[i] >> 27;
      result[i] *= const_values[2];
      result[i] ^= result[i] >> 33;
      result[i] *= const_values[4];
      result[i] ^= result[i] >> 33;
      final_result ^= result[i];
    }

    value = final_result;
  }

  constexpr Hash(S8 numeric) : value(hash_numeric(U64(numeric))) {}
  constexpr Hash(S16 numeric) : value(hash_numeric(U64(numeric))) {}
  constexpr Hash(S32 numeric) : value(hash_numeric(U64(numeric))) {}
  constexpr Hash(S64 numeric) : value(hash_numeric(U64(numeric))) {}
  constexpr Hash(U8 numeric) : value(hash_numeric(U64(numeric))) {}
  constexpr Hash(U16 numeric) : value(hash_numeric(U64(numeric))) {}
  constexpr Hash(U32 numeric) : value(hash_numeric(U64(numeric))) {}
  constexpr Hash(U64 numeric) : value(hash_numeric(U64(numeric))) {}
  template <typename pointer_type>
  Hash(pointer_type* pointer)
      : value(hash_numeric(U64(reinterpret_cast<CppSize>(pointer)))) {}

  // Escape constructor for custom types.
  template <typename hashable_type>
    requires(!__is_pointer(hashable_type))
  constexpr Hash(const hashable_type& obj) {
    value = obj.hash();
  }

  constexpr auto get_value() const -> U64 { return value; }

  // Rehash with an avalanch.
  constexpr auto Rehash(U64 hash) const -> U64 {
    U64 result;

    result = hash * const_values[0];
    result ^= result >> 27;
    result *= const_values[2];
    result ^= result >> 33;
    result *= const_values[4];
    result ^= result >> 33;
    result ^= value;
    return result;
  }

 private:
  // Number of 64 bit blocks to read per chunk.
  //
  // Valid options are 2 or 1 since generalizing to higher values didn't yeild
  // better results and just resulted in code bloat.
  static constexpr Count block_stride = 2;

  // Constants and indexes derived from feedback benchmarking.
  // Values may suffer from some overfitting to test data, but in practice
  // this setup seems to produce low collision rates per CPU cycle.
  static constexpr U64 const_values[5] = {
    0b00010101'01000101'10011011'00011001'10100101'11101101'01011101'10111011,
    0b10101010'10100011'01100000'11001011'00110001'10100110'00001101'01011101,
    0b00000000'00111011'11011100'01111101'11110101'11100110'00100001'01100101,
    0b01111111'11001100'01110011'01001100'10101010'01001010'10010001'11011101,
    0b00011111'11000010'00100001'01111001'00101110'01000000'11000001'10001001,
  };

  U64 value;
};

}  // namespace Perimortem::Core
