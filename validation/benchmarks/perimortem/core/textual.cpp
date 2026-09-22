// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/reader/textual.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static constexpr Count text_batch = 1024;

static Harness TextualBench = {
  .name = "Processing Text"_view,
};

PERIMORTEM_BENCHMARK(TextualBench, write_flags_x1024) {
  Static::Vector<Bool, 16> values = {
    {True, False, True, False, False, True, False, True, True, True, True,
     False, True, False, False, False}};
  Static::Bytes<20> buffer;
  for (Count i = 0; i < text_batch; i++) {
    const auto index = i & 0xF;
    Writer::Textual writer(buffer.get_access().slice(index, 20));
    writer << values[index];
  }

  Benchmark::prevent_optimization(buffer[0]);
}

PERIMORTEM_BENCHMARK(TextualBench, write_ints_x1024) {
  Static::Vector<S32, 16> values = {
    {10, 1401, -120, 987109, 3, -10, 242, 5131, -2147483648, 2147483647,
     -987654321, 123456789, 0, -1, 9999999, -100}};
  Count accumulator = 0;
  for (Count i = 0; i < text_batch; i++) {
    Static::Bytes<8> buffer;
    Writer::Textual writer(buffer.get_access());
    writer << values[i & 0xF];
    if (!writer.is_valid()) {
      accumulator += 1;
    } else {
      accumulator += writer.get_location();
    }
  }

  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(TextualBench, write_floats_x1024) {
  Static::Vector<R64, 16> values = {
    {1.0, -21.05, 781.012, 1200041.01, 2390.0037, 0.01234567, 890.098, 0.0,
     -0.000123, 9999.9999, 3.14159265, -2718.28, 812.99149, 29184.124,
     123456789.0, 0.123456789}};
  Count accumulator = 0;
  for (Count i = 0; i < text_batch; i++) {
    Static::Bytes<12> buffer;
    Writer::Textual writer(buffer.get_access());
    writer << values[i & 0xF];
    if (!writer.is_valid()) {
      accumulator += 1;
    } else {
      accumulator += writer.get_location();
    }
  }

  Benchmark::prevent_optimization(accumulator);
}

static Harness TextualReadBench = {
  .name = "Processing Text"_view,
};

PERIMORTEM_BENCHMARK(TextualReadBench, read_flags_x1024) {
  Static::Vector<View::Bytes, 16> values = {
    {"True"_view, "false"_view, "true"_view, "False"_view, "false"_view,
     "true"_view, "False"_view, "True"_view, "true"_view, "false"_view,
     "true"_view, "False"_view, "tree"_view, "folly"_view, "neither"_view,
     "unknown"_view}};
  Count accumulator = 0;
  for (Count i = 0; i < text_batch; i++) {
    Reader::Textual reader(values[i & 0xF]);
    Bool flag = reader.read_flag();
    accumulator += reader.has_content() ? flag ? 1 : 2 : 3;
  }

  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(TextualReadBench, read_ints_x1024) {
  Static::Vector<View::Bytes, 16> values = {
    {"10"_view, "1401"_view, "-120"_view, "987109"_view, "3"_view, "-10"_view,
     "242"_view, "5131"_view, "-2147483648"_view, "2147483647"_view,
     "-9876543210"_view, "1234567890123"_view, "number"_view, "abc"_view,
     "--42"_view, "12.5"_view}};
  Count accumulator = 0;
  for (Count i = 0; i < text_batch; i++) {
    Reader::Textual reader(values[i & 0xF]);
    Count long_value = reader.read_signed();
    if (reader.has_content()) {
      accumulator += long_value;
    } else {
      accumulator -= 1;
    }
  }

  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(TextualReadBench, read_floats_x1024) {
  Static::Vector<View::Bytes, 16> values = {
    {"1.0"_view, "-21.05"_view, "781.012"_view, "1200041.01"_view,
     "2390.0037"_view, "0.01234567"_view, "890.098"_view, "0"_view,
     "-0.000123"_view, "9999.9999"_view, "3.14159265"_view, "-2718.28"_view,
     "abc"_view, "1.2.3"_view, "xyz"_view, "number"_view}};
  R64 accumulator = 0.0;
  for (Count i = 0; i < text_batch; i++) {
    Reader::Textual reader(values[i & 0xF]);
    R64 real_value = reader.read_r64();
    if (reader.has_content()) {
      accumulator += real_value;
    } else {
      accumulator -= 1;
    }
  }

  Benchmark::prevent_optimization(accumulator);
}
