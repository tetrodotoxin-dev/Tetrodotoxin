// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/null_terminated.hpp"

#include "toolchain/validation/benchmark.hpp"

#include "perimortem/core/static/bytes.hpp"

#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;

static constexpr auto line = "value := 1234 + another_value;\n"_view;
static Static::Bytes<4096 * line.get_size()> text;
static Toolchain::Validation::Harness Source = {
  .name = "Tetrodotoxin::Source",
  .init =
      [] {
        for (Count i = 0; i < 4096; ++i) {
          Data::copy(
              text.get_data() + i * line.get_size(), line.get_data(),
              line.get_size());
        }
      },
};

// Measure the current Source owner without folding file acquisition or an
// unfinished dialect port into the tokenizer's cost.
VALIDATION_BENCHMARK(Source, tokenize_4096_lines) {
  Memory::Allocator::Arena arena;
  Tokenizer tokens(arena, text.get_view(), "benchmark.ttx"_view);
  auto count = tokens.get_size();
  Toolchain::Validation::Benchmark::prevent_optimization(count);
}
