// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/terminal.hpp"

#include "validation/unit_test.hpp"

#include <stdio.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Validation;

static Harness SystemTerminal = {
  .name = "System::Terminal"_view,
};

// Temporary streams keep production FILE behavior while giving each test an
// isolated byte sequence with no process global input or output replacement.
static auto open_stream(View::Bytes contents = View::Bytes()) -> FILE* {
  FILE* stream = tmpfile();
  if (stream == nullptr) {
    return nullptr;
  }

  if (!contents.is_empty()) {
    CppSize written;
    written = fwrite(contents.get_data(), 1, contents.get_size(), stream);
    if (written != contents.get_size()) {
      fclose(stream);
      return nullptr;
    }
  }

  S32 reset;
  reset = fseek(stream, 0, SEEK_SET);
  if (reset != 0) {
    fclose(stream);
    return nullptr;
  }

  return stream;
}

// The output oracle flushes and sizes the same stream before reading it back.
// This catches extra terminators and preserves embedded zero bytes exactly.
static auto read_stream(FILE& stream) -> Option<Dynamic::Bytes> {
  S32 flushed;
  flushed = fflush(&stream);
  if (flushed != 0) {
    return {};
  }

  S32 sought;
  sought = fseek(&stream, 0, SEEK_END);
  if (sought != 0) {
    return {};
  }

  S64 length;
  length = ftell(&stream);
  if (length < 0) {
    return {};
  }

  sought = fseek(&stream, 0, SEEK_SET);
  if (sought != 0) {
    return {};
  }

  Dynamic::Bytes contents;
  contents.resize(Count(length));
  if (contents.is_empty()) {
    return Option<Dynamic::Bytes>(Data::take(contents));
  }

  CppSize read;
  read =
      fread(contents.get_access().get_data(), 1, contents.get_size(), &stream);
  if (read != contents.get_size() || ferror(&stream) != 0) {
    return {};
  }

  return Option<Dynamic::Bytes>(Data::take(contents));
}

static auto matches(const Option<Dynamic::Bytes>& result, View::Bytes expected)
    -> Bool {
  return result.visit(
      []() -> Bool { return False; },
      [&](const Dynamic::Bytes& contents) -> Bool {
        return contents == expected;
      });
}

static auto close_stream(FILE* stream) -> void {
  if (stream != nullptr) {
    fclose(stream);
  }
}

PERIMORTEM_UNIT_TEST(SystemTerminal, blank_line) {
  FILE* input = open_stream("\n"_view);
  FILE* output = open_stream();
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  Terminal terminal(*input, *output);
  Option<Dynamic::Bytes> line;
  line = terminal.read_line();
  EXPECT(matches(line, ""_view));

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, immediate_eof) {
  FILE* input = open_stream();
  FILE* output = open_stream();
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  Terminal terminal(*input, *output);
  Option<Dynamic::Bytes> line;
  line = terminal.read_line();
  EXPECT(!line);

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, crlf_and_zero_bytes) {
  constexpr Static::Bytes<5> source = {{'a', 0, 'b', '\r', '\n'}};
  constexpr Static::Bytes<3> expected = {{'a', 0, 'b'}};
  FILE* input = open_stream(source);
  FILE* output = open_stream();
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  Terminal terminal(*input, *output);
  Option<Dynamic::Bytes> line;
  line = terminal.read_line();
  EXPECT(matches(line, expected));

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, final_line_at_eof) {
  FILE* input = open_stream("final\r"_view);
  FILE* output = open_stream();
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  Terminal terminal(*input, *output);
  Option<Dynamic::Bytes> line;
  line = terminal.read_line();
  EXPECT(matches(line, "final\r"_view));

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, long_line) {
  constexpr Count line_size = Count(1) << 16;
  Dynamic::Bytes source;
  source.append('x', line_size);
  source.append('\n');

  FILE* input = open_stream(source);
  FILE* output = open_stream();
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  Terminal terminal(*input, *output);
  Option<Dynamic::Bytes> line;
  line = terminal.read_line();
  EXPECT(matches(line, source.slice(0, line_size)));

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, sequential_lines) {
  FILE* input = open_stream("one\n\r\ntwo"_view);
  FILE* output = open_stream();
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  Terminal terminal(*input, *output);
  Option<Dynamic::Bytes> first;
  Option<Dynamic::Bytes> second;
  Option<Dynamic::Bytes> third;
  Option<Dynamic::Bytes> fourth;
  first = terminal.read_line();
  second = terminal.read_line();
  third = terminal.read_line();
  fourth = terminal.read_line();
  EXPECT(matches(first, "one"_view));
  EXPECT(matches(second, ""_view));
  EXPECT(matches(third, "two"_view));
  EXPECT(!fourth);

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, read_failure) {
  FILE* input = fopen("/dev/null", "w");
  FILE* output = open_stream();
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  Terminal terminal(*input, *output);
  Option<Dynamic::Bytes> line;
  line = terminal.read_line();
  EXPECT(!line);
  EXPECT(ferror(input) != 0);

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, exact_output) {
  constexpr Static::Bytes<3> source = {{'a', 0, 'b'}};
  constexpr Static::Bytes<4> expected = {{'a', 0, 'b', '\n'}};
  FILE* input = open_stream();
  FILE* output = open_stream();
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  Terminal terminal(*input, *output);
  Bool written;
  Option<Dynamic::Bytes> contents;
  written = terminal.write_line(source);
  contents = read_stream(*output);
  EXPECT(written);
  EXPECT(matches(contents, expected));

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, blank_output) {
  FILE* input = open_stream();
  FILE* output = open_stream();
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  Terminal terminal(*input, *output);
  Bool written;
  Option<Dynamic::Bytes> contents;
  written = terminal.write_line(View::Bytes());
  contents = read_stream(*output);
  EXPECT(written);
  EXPECT(matches(contents, "\n"_view));

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, short_write) {
  FILE* input = open_stream();
  FILE* output = fopen("/dev/full", "w");
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  S32 buffering;
  buffering = setvbuf(output, nullptr, _IONBF, 0);
  EXPECT_EQ(buffering, S32(0));

  Terminal terminal(*input, *output);
  Bool written;
  written = terminal.write_line("content"_view);
  EXPECT(!written);

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, newline_failure) {
  FILE* input = open_stream();
  FILE* output = fopen("/dev/full", "w");
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  S32 buffering;
  buffering = setvbuf(output, nullptr, _IONBF, 0);
  EXPECT_EQ(buffering, S32(0));

  Terminal terminal(*input, *output);
  Bool written;
  written = terminal.write_line(View::Bytes());
  EXPECT(!written);

  close_stream(input);
  close_stream(output);
}

PERIMORTEM_UNIT_TEST(SystemTerminal, flush_failure) {
  Static::Bytes<128> buffer;
  FILE* input = open_stream();
  FILE* output = fopen("/dev/full", "w");
  EXPECT(input != nullptr);
  EXPECT(output != nullptr);
  if (input == nullptr || output == nullptr) {
    close_stream(input);
    close_stream(output);
    return;
  }

  S32 buffering;
  buffering = setvbuf(output, Data::cast<char>(buffer.get_data()), _IOFBF, 128);
  EXPECT_EQ(buffering, S32(0));

  Terminal terminal(*input, *output);
  Bool written;
  written = terminal.write_line("content"_view);
  EXPECT(!written);

  close_stream(input);
  close_stream(output);
}
