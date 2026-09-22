// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/hash.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/system/random.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

// A large batch amortizes timer overhead for one benchmark invocation.
static constexpr Count hash_batch = 8192;

// The largest retained hash is 256 bytes and slides through eight offsets.
static Static::Bytes<264> hash_buffer;

// Make sure keys are randomized
template <typename byte_array>
static auto fill_hash(byte_array& target) -> void {
  Count* data = Data::cast<Count>(target.get_data());
  for (Count i = 0; i < target.get_size() / 8; i++) {
    data[i] = Random::generate();
  }
}

static Harness HashBench = {
  .name = "Hashing"_view,
  .setup = []() { fill_hash(hash_buffer); },
};

// Create a data dependency which disables Clang SIMD optimization so we can get
// a feel for scalar performance since in real use hashes tend to be performed
// as part of a hot path and it's not typical to vectorize over a range of a
// thousand keys in one go.
PERIMORTEM_BENCHMARK(HashBench, u32_x8192) {
  U32 input = Data::cast<U32>(hash_buffer.get_data())[0];
  U64 accumulator = 0;
  for (Count i = 0; i < hash_batch; i++) {
    U64 result = Hash(input).get_value();
    accumulator ^= result;
    input = U32(result);
  }

  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(HashBench, u64_x8192) {
  U64 input = Data::cast<U64>(hash_buffer.get_data())[0];
  U64 accumulator = 0;
  for (Count i = 0; i < hash_batch; i++) {
    U64 result = Hash(input).get_value();
    accumulator ^= result;
    input = result;
  }

  Benchmark::prevent_optimization(accumulator);
}

template <Count hash_length>
static auto compute_hash() -> void {
  // Slide the window by one byte per iteration so the optimizer cannot prove
  // all calls return the same value and fold the XOR chain to zero.
  constexpr Count max_offset = 8;
  U64 accumulator = 0;
  for (Count i = 0; i < hash_batch; i++) {
    Count offset = (max_offset > 0) ? (i % (max_offset + 1)) : 0;
    accumulator ^= Hash(hash_buffer.slice(offset, hash_length)).get_value();
  }

  Benchmark::prevent_optimization(accumulator);
}

#define HASH_BENCH(key_length)                                       \
  PERIMORTEM_BENCHMARK(HashBench, key_length_##key_length##_x8192) { \
    compute_hash<key_length>();                                      \
  }

HASH_BENCH(64);
HASH_BENCH(128);
HASH_BENCH(256);
