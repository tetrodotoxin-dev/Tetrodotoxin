// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/benchmark.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static constexpr Count string_batch = 1024;
static Harness StringBench = {
  .name = "Strings"_view,
};

template <Count byte_length>
static auto concat_test() -> void {
  Count size = 0;
  for (Count i = 0; i < string_batch; i++) {
    Dynamic::Bytes buffer;
    buffer.concat(Static::Bytes<byte_length>());
    size += buffer.get_size();
  }

  Benchmark::prevent_optimization(size);
}

#define CONCAT_TEST(size)                                    \
  PERIMORTEM_BENCHMARK(StringBench, concat_##size##_x1024) { \
    concat_test<size>();                                     \
  }

CONCAT_TEST(8);
CONCAT_TEST(16);
CONCAT_TEST(32);
CONCAT_TEST(64);
CONCAT_TEST(128);

PERIMORTEM_BENCHMARK(StringBench, append_1024_bytes) {
  Dynamic::Bytes buffer;
  constexpr auto append_count = 1 << 10;
  for (Count i = 0; i < append_count; i++) {
    buffer.append('0');
  }

  Count size = buffer.get_size();
  Benchmark::prevent_optimization(size);
  buffer.reset();
}

PERIMORTEM_BENCHMARK(StringBench, small_string_x1024) {
  Count size = 0;
  for (Count i = 0; i < string_batch; i++) {
    Dynamic::Bytes buffer = "small string"_view;
    size += buffer.get_size();
  }

  Benchmark::prevent_optimization(size);
}

PERIMORTEM_BENCHMARK(StringBench, small_concat_x1024) {
  Count size = 0;
  for (Count i = 0; i < string_batch; i++) {
    Dynamic::Bytes buffer = "small"_view;
    buffer.concat(" string"_view);
    size += buffer.get_size();
  }

  Benchmark::prevent_optimization(size);
}
