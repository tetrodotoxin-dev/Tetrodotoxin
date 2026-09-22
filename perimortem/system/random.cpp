// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/random.hpp"

#include <immintrin.h>

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;

static constexpr Count channel_depth = 4;
static constexpr Count max_index =
    sizeof(__m256i) / sizeof(U64) * channel_depth;

struct PhiloxState {
  static constexpr Count round_count = 10;

  // Philox4x32 uses two multiplication constants and advances its key with two
  // Weyl constants after each round. Duplicating those pairs across the AVX2
  // lanes evaluates two independent Philox generators per vector.
  static constexpr __m256i philox4x32_constants = _mm256_set_epi64x(
      S64(0x00000000'D2511F53),
      S64(0x00000000'CD9E8D57),
      S64(0x00000000'D2511F53),
      S64(0x00000000'CD9E8D57));
  static constexpr __m256i philox4x32_xor_mask = _mm256_set_epi64x(
      S64(0xFFFFFFFF'00000000),
      S64(0xFFFFFFFF'00000000),
      S64(0xFFFFFFFF'00000000),
      S64(0xFFFFFFFF'00000000));
  static constexpr __m256i philox4x32_weyl = _mm256_set_epi64x(
      S64(0x9E2779B9'00000000),
      S64(0xBB67AE85'00000000),
      S64(0x9E2779B9'00000000),
      S64(0xBB67AE85'00000000));
  // Reorders the multiplied counter halves for the next round. The high half
  // crosses each 64 bit pair while the low half moves into the high position.
  static constexpr U8 counter_shuffle = 0b10'01'00'11;

  alignas(32) U64 output[max_index];
  __m256i dual_channel_key;
  __m256i dual_channel_counter;
  Count index;
};

auto Random::read_entropy() -> U64 {
  U64 value;
  Count timeout = 100000;
  while (!_rdrand64_step(&value) and timeout) {
    timeout -= 1;
  }

  // RDRAND can transiently fail. The bounded retry avoids hanging startup. The
  // C runtime fallback is only a last resort seed source and must not be
  // treated as cryptographic entropy.
  if (timeout == 0) {
    return (Count(rand()) << 32) | Count(rand());
  }

  return value;
}

// Advances four counter depths for each of the two vectorized Philox channels.
//
// One refill produces sixteen 64 bit values. All four depths must pass through
// every Philox round. Leaving depth zero as the raw counter would preserve
// uniqueness while destroying the statistical meaning of the generator.
static constexpr auto bump_counter(PhiloxState& state) -> void {
  __m256i philox_keys = state.dual_channel_key;
  __m256i philox_channels[channel_depth];
  philox_channels[0] = state.dual_channel_counter;
  for (Count i = 1; i < channel_depth; i++) {
    philox_channels[i] =
        _mm256_add_epi64(state.dual_channel_counter, _mm256_set1_epi64x(i));
  }

  for (Count round = 0; round < PhiloxState::round_count; round++) {
    for (Count i = 0; i < channel_depth; i++) {
      const auto hilo_mul = _mm256_mul_epu32(
          philox_channels[i], PhiloxState::philox4x32_constants);
      const auto xor_mask = _mm256_and_si256(
          philox_channels[i], PhiloxState::philox4x32_xor_mask);
      const auto eval = _mm256_xor_si256(hilo_mul, xor_mask);
      philox_channels[i] = _mm256_shuffle_epi32(
          _mm256_xor_si256(eval, philox_keys), PhiloxState::counter_shuffle);
    }

    if (round != PhiloxState::round_count - 1) {
      philox_keys = _mm256_add_epi32(philox_keys, PhiloxState::philox4x32_weyl);
    }
  }

  for (Count i = 0; i < channel_depth; i++) {
    _mm256_store_si256(
        Data::cast<__m256i>(state.output) + i, philox_channels[i]);
  }

  // The next refill starts after every counter consumed by this batch.
  state.dual_channel_counter = _mm256_add_epi64(
      state.dual_channel_counter, _mm256_set1_epi64x(channel_depth));
  state.index = 0;
}

// Seeds independent keys and counters for one thread local generator. Keeping
// the state thread local avoids synchronization and false sharing in the hot
// generate() path.
static auto create_prng() -> PhiloxState {
  PhiloxState state;

  U64 keys[] = {Random::read_entropy(), Random::read_entropy()};
  state.dual_channel_key = _mm256_set_epi32(
      U32(keys[0] >> 32), 0, U32(keys[0]), 0, U32(keys[1] >> 32), 0,
      U32(keys[1]), 0);

  state.dual_channel_counter = _mm256_set_epi64x(
      Random::read_entropy(), Random::read_entropy(), Random::read_entropy(),
      Random::read_entropy());

  bump_counter(state);
  return state;
}

auto Random::generate() -> U64 {
  thread_local static PhiloxState engine = create_prng();
  if (engine.index == max_index) {
    bump_counter(engine);
  }

  return engine.output[engine.index++];
}
