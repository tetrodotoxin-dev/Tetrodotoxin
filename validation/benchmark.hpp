// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "validation/harness.hpp"

namespace Validation::Benchmark {

using BenchmarkFunc = void (*)();

// Sets a new start point inside a benchmark body. This excludes preparation
// that cannot live in the Harness setup callback.
auto start_time() -> void;

// Lets a benchmark body override the end timestamp.
auto end_time() -> void;

auto create(
    const Harness& harness,
    Perimortem::Core::View::Bytes name,
    BenchmarkFunc func) -> void;

// Prevents the optimizer from removing test loads using a read/write
// register or memory constraint.
template <typename value_type>
auto prevent_optimization(value_type& value) -> void {
  asm volatile("" : "+r,m"(value) : : "memory");
}

class BenchmarkEntry {
 public:
  BenchmarkEntry(
      const Harness& harness,
      Perimortem::Core::View::Bytes name,
      BenchmarkFunc func) {
    create(harness, name, func);
  }
};

}  // namespace Validation::Benchmark

#define PERIMORTEM_BENCHMARK(harness, name)                                    \
  static auto benchmark_##harness##_##name() -> void;                          \
  namespace {                                                                  \
  Validation::Benchmark::BenchmarkEntry benchmark_entry_##harness##_##name = { \
    harness, #name##_view, benchmark_##harness##_##name};                      \
  }                                                                            \
  static auto benchmark_##harness##_##name() -> void
