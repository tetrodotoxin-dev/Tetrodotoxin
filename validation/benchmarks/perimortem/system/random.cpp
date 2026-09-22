// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/random.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/perimortem.hpp"

using namespace Perimortem::System;
using namespace Validation;

// Large batch to amortise the clock_gettime overhead — Philox is extremely fast
// so a single call produces results in under a nanosecond.
static constexpr Count random_batch = 1024;

static Harness SystemRandom = {
  .name = "Random"_view,
};

PERIMORTEM_BENCHMARK(SystemRandom, generate_1024) {
  // Throughput benchmark: 1024 Philox derived 64 bit values.
  U64 accumulator = 0;
  for (Count batch_index = 0; batch_index < random_batch; batch_index++) {
    accumulator ^= Random::generate();
  }

  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(SystemRandom, read_entropy_1024) {
  // Throughput benchmark: 1024 OS entropy reads.
  U64 accumulator = 0;
  for (Count batch_index = 0; batch_index < random_batch; batch_index++) {
    accumulator ^= Random::read_entropy();
  }

  Benchmark::prevent_optimization(accumulator);
}
