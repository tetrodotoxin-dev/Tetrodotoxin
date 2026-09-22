// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/core/static/bytes.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;

#include <x86intrin.h>

auto Algorithm::search(View::Bytes src, View::Bytes value) -> Count {
  // Fall back to the more optimized value check if the View::Bytes is only a
  // single byte long. This avoids all of the extra tail checks.
  if (value.get_size() == 1) {
    return search(src, value[0]);
  }

  // If the value is larger than the source then it can't be a substring.
  if (value.get_size() > src.get_size()) {
    return Count(-1);
  }

  // If the value and src are the same size then we just have to test if they
  // are the same object.
  if (value.get_size() == src.get_size()) {
    return value == src ? 0 : Count(-1);
  }

  // Setup two additional registers with the exact value test as well as the
  // test mask. Since the source can be any length this is easier to setup by
  // loading from two Static::Bytes.
  //
  // Since we can find matches close to the end of the array we need twice the
  // vector size in order to ensure we have valid padding.
  Count i = 0;
  const Count tail_offset = value.get_size() - 1;
  constexpr auto vectorize_limit = sizeof(__m256i);
  if (src.get_size() >= vectorize_limit * 2 &&
      value.get_size() <= vectorize_limit) [[likely]] {
    Static::Bytes<vectorize_limit> value_bytes = value;
    Static::Bytes<vectorize_limit> value_filter;
    Data::set(value_filter.get_data(), 0xFF, value.get_size());
    const auto first_byte = _mm256_set1_epi8(value[0]);
    const auto last_byte = _mm256_set1_epi8(value[tail_offset]);
    const auto test_value =
        _mm256_loadu_si256(Data::cast<const __m256i_u>(value_bytes.get_data()));
    const auto test_filter = _mm256_loadu_si256(
        Data::cast<const __m256i_u>(value_filter.get_data()));

    // loop through all vectorizable chunks possible.
    // Each chunk tests for 32 possible valid locations based on start and end
    // pairings which performs vastly better than just checking for start values
    // on long ranges.
    for (; i < src.get_size() - vectorize_limit * 2; i += vectorize_limit) {
      const auto head_block =
          _mm256_loadu_si256(Data::cast<const __m256i_u>(src.get_data() + i));
      const auto tail_block = _mm256_loadu_si256(
          Data::cast<const __m256i_u>(src.get_data() + i + tail_offset));

      const auto head_slots = _mm256_cmpeq_epi8(head_block, first_byte);
      const auto tail_slots = _mm256_cmpeq_epi8(tail_block, last_byte);
      const auto test_ranges = _mm256_and_si256(head_slots, tail_slots);
      auto range_mask = U32(_mm256_movemask_epi8(test_ranges));
      while (range_mask) {
        auto index = __builtin_ctzg(range_mask);
        range_mask ^= 1 << index;

        // Load the block and filter it down to only the section we care about.
        const auto test_block = _mm256_loadu_si256(
            Data::cast<const __m256i_u>(src.get_data() + i + index));
        const auto possible_match = _mm256_and_si256(test_block, test_filter);
        const auto match_value = _mm256_cmpeq_epi8(possible_match, test_value);
        auto match = U32(_mm256_movemask_epi8(match_value));
        if (match == 0xFFFFFFFF) {
          return i + index;
        }
      }
    }
  }

  // Scalar fallback using head/tail checking.
  for (; i < src.get_size() - tail_offset; i++) {
    // Skip unless both head and tail bytes match. Any mismatch rules out this
    // position without touching the middle bytes.
    if (src[i] != value[0] || src[i + tail_offset] != value[tail_offset]) {
      continue;
    }

    Bool found = True;
    for (Count j = 1; j < tail_offset; j++) {
      if (src[i + j] != value[j]) {
        found = False;
        break;
      }
    }

    if (found) {
      return i;
    }
  }

  // Not found
  return Count(-1);
}

// Fast vectorized sub string search for a particular byte in a View::Bytes.
auto Algorithm::search(View::Bytes src, U8 value) -> Count {
  // If the value is larger than the source then it can't be a substring.
  if (src.is_empty()) {
    return Count(-1);
  }

  // Do a straight forward scan since we can do direct epi8 tests rather than
  // sub ranges.
  Count i = 0;
  auto source_data = src.get_data();
  constexpr auto vectorize_limit = sizeof(__m256i);
  if (src.get_size() >= vectorize_limit) [[likely]] {
    const auto test_mask = _mm256_set1_epi8(value);

    // loop through all vectorizable chunks possible.
    // Each chunk tests for 32 possible valid locations based on start and end
    // pairings which performs vastly better than just checking for start values
    // on long ranges.
    for (; i < src.get_size() - vectorize_limit; i += vectorize_limit) {
      const auto source_block =
          _mm256_loadu_si256(Data::cast<const __m256i_u>(source_data + i));

      const auto source_hits = _mm256_cmpeq_epi8(source_block, test_mask);
      const auto range_mask = U32(_mm256_movemask_epi8(source_hits));
      if (range_mask) {
        auto index = __builtin_ctzg(range_mask);
        return i + index;
      }
    }
  }

  // Scalar fallback.
  for (; i < src.get_size(); i++) {
    if (value == source_data[i]) {
      return i;
    }
  }

  // Not found
  return Count(-1);
}
