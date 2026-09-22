// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// benchmark runner — analogous to validation/unit_test.cpp but for
// performance measurement rather than correctness. Each benchmark is called
// repeatedly until a wall clock cap is reached. Timing samples are sorted and
// split into three percentile buckets to distinguish typical from outlier
// performance.

#include "validation/benchmark.hpp"

#include <stdio.h>

#include "perimortem/core/access/vector.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/sort.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/time.hpp"

using namespace Validation;
using namespace Perimortem::Core;

constexpr const char* clear_color = "\x1b[0m";
constexpr const char* perimortem_color = "\x1b[38;5;124m";
constexpr const char* dark_color = "\x1b[38;5;238m";
constexpr const char* system_color = "\x1b[38;5;246m";
constexpr const char* fast_color = "\x1b[38;5;34m";
constexpr const char* slow_color = "\x1b[38;5;160m";

struct BenchmarkInstance {
  const Harness* harness;
  Perimortem::Core::View::Bytes name;
  Benchmark::BenchmarkFunc func;
};

struct SampleStats {
  U64 bottom_avg_ns;
  U64 middle_avg_ns;
  U64 top_avg_ns;
  U64 allocation_requests;
};

static constexpr Count max_benchmark_count = 1024;
static constexpr Count max_sample_count = 4096;
static constexpr R64 time_cap_sec = 1.5;

static Static::Vector<BenchmarkInstance, max_benchmark_count> benchmarks;
static Static::Vector<U64, max_sample_count> time_samples;
static Count benchmark_count = 0;
static View::Bytes benchmark_filter = {};

auto Benchmark::create(
    const Harness& harness,
    Perimortem::Core::View::Bytes name,
    Benchmark::BenchmarkFunc func) -> void {
  benchmarks[benchmark_count++] = {&harness, name, func};
}

// Returns a View::Bytes into buffer with the formatted time string.
static auto format_time(Static::Bytes<16>& buffer, U64 ns) -> View::Bytes {
  auto* character_buffer = Data::cast<char>(buffer.get_data());
  int written = 0;
  if (ns < 1'000ULL) {
    written = snprintf(
        character_buffer, buffer.get_size(), "%llu ns", (unsigned long long)ns);
  } else if (ns < 1'000'000ULL) {
    written = snprintf(
        character_buffer, buffer.get_size(), "%.2f us", R64(ns) / 1'000.0);
  } else {
    written = snprintf(
        character_buffer, buffer.get_size(), "%.2f ms", R64(ns) / 1'000'000.0);
  }

  return View::Bytes(buffer.get_data(), Count(written > 0 ? written : 0));
}

static auto bucket_avg(Count start, Count end_index) -> U64 {
  if (start >= end_index) {
    return time_samples[end_index > 0 ? end_index - 1 : 0];
  }

  U64 total = 0;
  for (Count index = start; index < end_index; index++) {
    total += time_samples[index];
  }

  return total / (end_index - start);
}

static auto compute_stats(Count sample_count, U64 alloc_requests)
    -> SampleStats {
  SampleStats stats = {};
  stats.allocation_requests = alloc_requests;

  Count tenth = sample_count / 10;
  if (tenth < 1) {
    tenth = 1;
  }

  stats.bottom_avg_ns = bucket_avg(0, tenth);
  stats.middle_avg_ns = bucket_avg(tenth, sample_count - tenth);
  stats.top_avg_ns = bucket_avg(sample_count - tenth, sample_count);
  return stats;
}

static auto print_stats(
    View::Bytes name,
    Count col_width,
    const SampleStats& stats) -> void {
  Static::Bytes<16> bottom_buffer, middle_buffer, top_buffer;
  View::Bytes bottom = format_time(bottom_buffer, stats.bottom_avg_ns);
  View::Bytes middle = format_time(middle_buffer, stats.middle_avg_ns);
  View::Bytes top = format_time(top_buffer, stats.top_avg_ns);

  printf(
      "  %-*.*s %s%9.*s%s  %9.*s  %s%9.*s%s", (int)col_width,
      (int)name.get_size(), Data::cast<char>(name.get_data()), fast_color,
      (int)bottom.get_size(), Data::cast<char>(bottom.get_data()), clear_color,
      (int)middle.get_size(), Data::cast<char>(middle.get_data()), slow_color,
      (int)top.get_size(), Data::cast<char>(top.get_data()), clear_color);
  if (stats.allocation_requests > 0) {
    printf(
        "  | %s%lld alloc/run%s", system_color,
        (long long)stats.allocation_requests, clear_color);
  }

  printf("\n");
}

static auto output_break() -> void {
  printf(
      "%s[==============================================================]\n%s",
      dark_color, clear_color);
}

// Timing blocks
static Time total_start;
static Time sample_start;
static Time sample_end;

auto Benchmark::start_time() -> void {
  sample_end = Time::never();
  sample_start = Time::now();
}

auto Benchmark::end_time() -> void {
  sample_end = Time::now();
}

static auto run_samples(const Harness& harness, Benchmark::BenchmarkFunc func)
    -> SampleStats {
  // Perform one run as a warm up.
  harness.setup();
  func();
  harness.teardown();

  Count sample_count = 0;
  U64 total_alloc_delta = 0;
  total_start = Time::now();
  while (sample_count < max_sample_count) {
    harness.setup();
    Count allocs_before = Bibliotheca::check_out_requests();
    Benchmark::start_time();

    // Run the benchmark body.
    func();

    // Preserve an end time recorded by the benchmark body.
    if (sample_end == Time::never()) {
      Benchmark::end_time();
    }

    // Grab the number of allocations and tear down the test.
    Count allocs_after = Bibliotheca::check_out_requests();
    harness.teardown();

    time_samples[sample_count++] =
        sample_start.measure(sample_end).convert_to_nanoseconds();
    total_alloc_delta += U64(allocs_after - allocs_before);

    // Check the time budget every 16 samples so the clock check does not
    // dominate small workloads.
    if ((sample_count & 0xF) == 0) {
      if (total_start.measure().convert_to_seconds() >= time_cap_sec) {
        break;
      }
    }
  }

  Algorithm::sort(Access::Vector<U64>(time_samples.get_data(), sample_count));
  return compute_stats(sample_count, total_alloc_delta / U64(sample_count));
}

static auto harness_matches(View::Bytes name) -> Bool {
  if (benchmark_filter.get_size() == 0) {
    return True;
  }

  if (benchmark_filter.get_size() > name.get_size()) {
    return False;
  }

  for (Count i = 0; i < benchmark_filter.get_size(); i++) {
    if ((benchmark_filter.get_data()[i] | 0x20) !=
        (name.get_data()[i] | 0x20)) {
      return False;
    }
  }

  return True;
}

struct Layout {
  Count col_width;
  Count harness_count;
};

static auto compute_layout() -> Layout {
  Count col_width = 16;
  Count harness_count = 0;
  const Harness* prev_harness = nullptr;
  for (Count index = 0; index < benchmark_count; index++) {
    if (!harness_matches(benchmarks[index].harness->name)) {
      continue;
    }

    Count name_length = benchmarks[index].name.get_size();
    if (name_length > col_width) {
      col_width = name_length;
    }

    if (benchmarks[index].harness != prev_harness) {
      harness_count++;
      prev_harness = benchmarks[index].harness;
    }
  }

  return {col_width + 2, harness_count};
}

static auto print_run_header(const Layout& layout) -> void {
  output_break();
  printf(
      "%s  Perimortem Benchmark Runner\n"
      "  benchmarks: %s%llu%s   Harnesses: %s%llu%s\n",
      perimortem_color, clear_color, (unsigned long long)benchmark_count,
      system_color, clear_color, (unsigned long long)layout.harness_count,
      clear_color);
  printf(
      "%s  Columns: p0-10 (fast)  p10-90 (typical)  p90-100 (outliers)\n"
      "  Build with -c opt for meaningful numbers%s\n",
      system_color, clear_color);
  output_break();
  printf(
      "%s  %-*s %s p0-10  %s   p10-90   %s p90-100%s\n", dark_color,
      (int)layout.col_width, "", fast_color, dark_color, slow_color,
      clear_color);
}

static auto run_benchmark_pass(const Layout& layout) -> void {
  const Harness* active_harness = nullptr;
  const Harness* output_harness = nullptr;
  for (Count benchmark_index = 0; benchmark_index < benchmark_count;
       benchmark_index++) {
    const BenchmarkInstance& benchmark = benchmarks[benchmark_index];
    if (benchmark.harness == nullptr) {
      continue;
    }

    const Harness& harness = *benchmark.harness;
    if (!harness_matches(harness.name)) {
      continue;
    }

    if (active_harness != benchmark.harness) {
      active_harness = benchmark.harness;
      active_harness->init();
    }

    if (output_harness == nullptr || harness.name != output_harness->name) {
      output_harness = benchmark.harness;
      printf(
          "%s[ START ] %.*s\n%s", dark_color, (int)harness.name.get_size(),
          Data::cast<char>(harness.name.get_data()), clear_color);
    }

    SampleStats stats = run_samples(harness, benchmark.func);
    print_stats(benchmark.name, layout.col_width, stats);
  }
}

int main(int argc, const char* argv[]) {
  if (argc > 1) {
    benchmark_filter = NullTerminated::to_view(argv[1]);
  }

  Layout layout = compute_layout();
  print_run_header(layout);
  run_benchmark_pass(layout);
  output_break();
  printf("\n");
  fflush(stdout);
  return 0;
}
