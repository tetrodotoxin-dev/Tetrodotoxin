// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/benchmark.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/random.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Validation;

static Harness AllocatorBench = {
  .name = "Allocators"_view,
};

template <Count alloc_size>
static auto bibliotheca_cycle() -> void {
  Bibliotheca::Allocation alloc;
  for (Count i = 0; i < 100; i++) {
    alloc = Bibliotheca::check_out(alloc_size);
    Benchmark::prevent_optimization(alloc.ptr);
    Bibliotheca::remit(alloc.ptr);
  }
}

template <Count alloc_size>
static auto arena_cycle() -> void {
  Allocator::Arena arena;
  for (Count i = 0; i < 100; i++) {
    auto alloc = arena.allocate(alloc_size);
    Benchmark::prevent_optimization(alloc);
  }
}

#define CYCLE_BENCH(size)                                          \
  PERIMORTEM_BENCHMARK(AllocatorBench, bibliotheca_cycle_##size) { \
    bibliotheca_cycle<size>();                                     \
  }                                                                \
  PERIMORTEM_BENCHMARK(AllocatorBench, arena_cycle_##size) {       \
    arena_cycle<size>();                                           \
  }

CYCLE_BENCH(64)
CYCLE_BENCH(1024)
CYCLE_BENCH(65536)

template <Count frame_alloc_count, Count size_minimum, Count size_range>
static auto frame_stability() -> void {
  Static::Vector<U8*, frame_alloc_count> ptrs;
  for (Count i = 0; i < frame_alloc_count; i++) {
    auto alloc = Bibliotheca::check_out(
        (Random::generate() & size_range) + size_minimum);
    ptrs[i] = alloc.ptr;
  }

  for (Count i = 0; i < frame_alloc_count; i++) {
    Bibliotheca::remit(ptrs[i]);
  }

  Benchmark::prevent_optimization(ptrs[0]);
}

#define STABILITY_BENCH(prefix, count, min_bytes, max_mask)     \
  PERIMORTEM_BENCHMARK(                                         \
      AllocatorBench, frame_##prefix##_##count##_allocations) { \
    frame_stability<count, min_bytes, max_mask>();              \
  }

STABILITY_BENCH(small, 1024, 64, 0xFF);
STABILITY_BENCH(small, 65536, 64, 0xFF);
STABILITY_BENCH(varied, 1024, 64, 0xFFFF);

template <
    Count frame_alloc_count,
    Count window_count,
    Count size_minimum,
    Count size_range>
static auto frame_stability_interleaved() -> void {
  constexpr Count window = frame_alloc_count / window_count;
  Static::Vector<U8*, frame_alloc_count> ptrs;

  // Initial allocation.
  for (Count i = 0; i < window; i++) {
    ptrs[i] =
        Bibliotheca::check_out((Random::generate() & size_range) + size_minimum)
            .ptr;
  }

  // Rolling exchange.
  for (Count i = window; i < frame_alloc_count; i++) {
    ptrs[i] =
        Bibliotheca::check_out((Random::generate() & size_range) + size_minimum)
            .ptr;
    Bibliotheca::remit(ptrs[i - window]);
  }

  // Final clean up.
  for (Count i = frame_alloc_count - window; i < frame_alloc_count; i++) {
    Bibliotheca::remit(ptrs[i]);
  }

  Benchmark::prevent_optimization(ptrs[0]);
}

#define INTERLEAVED_BENCH(count, windows)                                      \
  PERIMORTEM_BENCHMARK(                                                        \
      AllocatorBench, frame_##windows##_interleaved##_##count##_allocations) { \
    frame_stability_interleaved<count, windows, 64, 0xFF>();                   \
  }

INTERLEAVED_BENCH(16384, 8);
INTERLEAVED_BENCH(16384, 32);
INTERLEAVED_BENCH(65536, 8);
INTERLEAVED_BENCH(65536, 32);
