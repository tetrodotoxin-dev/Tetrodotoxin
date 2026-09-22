// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "validation/process/fixture.hpp"

#include <stdio.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/time.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Validation;

constexpr const char* clear_color = "\x1b[0m";
constexpr const char* perimortem_color = "\x1b[38;5;124m";
constexpr const char* dark_color = "\x1b[38;5;238m";
constexpr const char* system_color = "\x1b[38;5;246m";
constexpr const char* fail_color = "\x1b[38;5;160m";
constexpr const char* pass_color = "\x1b[38;5;34m";

struct Instance {
  const Harness* harness;
  Perimortem::Core::View::Bytes name;
  Test::TestFunc func;
  Perimortem::Core::View::Bytes file;
  Count line;
};

static Diagnostics::Log::Level captured_log_level;
static Static::Bytes<2048> captured_log_message;
static Count captured_log_message_size = 0;

static Static::Vector<Instance, 4096> binary_tests;
static Static::Vector<Count, 4096> failed_test_indexes;
static Count binary_tests_count = 0;
static Count test_suites = 0;
static Count passed_tests = 0;
static Count failed_tests = 0;
static Count not_run_tests = 0;

static constexpr View::Bytes actual_label = "    ACTUAL = "_view;
static constexpr View::Bytes expected_label = "  EXPECTED = "_view;

auto Test::capture_sink(
    Diagnostics::Log::Level level,
    View::Bytes message,
    const Diagnostics::Source&) -> void {
  captured_log_level = level;
  captured_log_message_size = message.get_size();
  captured_log_message = message;
}

auto Test::captured_message() -> View::Bytes {
  return captured_log_message.slice(0, captured_log_message_size);
}

auto Test::error_contains(View::Bytes message, Diagnostics::Log::Level level)
    -> Bool {
  if (captured_log_level != level) {
    return false;
  }

  return Algorithm::search(captured_message(), message) != Count(-1);
}

auto Test::log_message(View::Bytes file, Count line, View::Bytes message)
    -> void {
  printf(
      "%.*s:%llu:\n    %.*s\n", (int)file.get_size(), file.get_data(),
      (unsigned long long)line, (int)message.get_size(), message.get_data());
}

auto Test::create(
    const Harness& harness,
    Perimortem::Core::View::Bytes name,
    TestFunc func,
    Perimortem::Core::View::Bytes file,
    Count line) -> void {
  binary_tests[binary_tests_count++] = {&harness, name, func, file, line};
}

static auto write_label(Bool actual) -> void {
  auto label = actual ? actual_label : expected_label;
  fwrite(label.get_data(), 1, label.get_size(), stdout);
}

auto Test::expected(Bool value, Bool actual) -> void {
  Static::Bytes<32> buffer;
  Writer::Textual text(buffer.get_access());
  text << (actual ? actual_label : expected_label) << value << "\n"_view;
  fwrite(buffer.get_data(), 1, text.get_location(), stdout);
}

auto Test::expected(View::Bytes value, Bool actual) -> void {
  write_label(actual);
  fwrite(value.get_data(), 1, value.get_size(), stdout);
  putchar('\n');
}

auto Test::expected(U16 value, Bool actual) -> void {
  Static::Bytes<32> buffer;
  Writer::Textual text(buffer.get_access());
  text << (actual ? actual_label : expected_label) << value << "\n"_view;
  fwrite(buffer.get_data(), 1, text.get_location(), stdout);
}

auto Test::expected(U32 value, Bool actual) -> void {
  Static::Bytes<32> buffer;
  Writer::Textual text(buffer.get_access());
  text << (actual ? actual_label : expected_label) << value << "\n"_view;
  fwrite(buffer.get_data(), 1, text.get_location(), stdout);
}

auto Test::expected(U64 value, Bool actual) -> void {
  Static::Bytes<32> buffer;
  Writer::Textual text(buffer.get_access());
  text << (actual ? actual_label : expected_label) << value << "\n"_view;
  fwrite(buffer.get_data(), 1, text.get_location(), stdout);
}

auto Test::expected(S16 value, Bool actual) -> void {
  Static::Bytes<32> buffer;
  Writer::Textual text(buffer.get_access());
  text << (actual ? actual_label : expected_label) << value << "\n"_view;
  fwrite(buffer.get_data(), 1, text.get_location(), stdout);
}

auto Test::expected(S32 value, Bool actual) -> void {
  Static::Bytes<32> buffer;
  Writer::Textual text(buffer.get_access());
  text << (actual ? actual_label : expected_label) << value << "\n"_view;
  fwrite(buffer.get_data(), 1, text.get_location(), stdout);
}

auto Test::expected(S64 value, Bool actual) -> void {
  Static::Bytes<32> buffer;
  Writer::Textual text(buffer.get_access());
  text << (actual ? actual_label : expected_label) << value << "\n"_view;
  fwrite(buffer.get_data(), 1, text.get_location(), stdout);
}

auto Test::expected(CppSize value, Bool actual) -> void {
  expected(S64(value), actual);
}

auto Test::expected(R64 value, Bool actual) -> void {
  Static::Bytes<48> buffer;
  Writer::Textual text(buffer.get_access());
  text << (actual ? actual_label : expected_label) << value << "\n"_view;
  fwrite(buffer.get_data(), 1, text.get_location(), stdout);
}

auto Test::expected_text(View::Bytes value, View::Bytes other, Bool actual)
    -> void {
  write_label(actual);

  // Loop over all the bytes and flag any diffs.
  // For an extra or missing byte a string of red is generated.
  // For mismatched bytes you get the exact byte.
  // Not perfect or anything, but hey it gets the job done for most cases!
  for (Count i = 0; i < value.get_size(); i++) {
    if (i >= other.get_size() || value[i] != other[i]) {
      printf("%s%c", fail_color, value[i]);
    } else {
      printf("%s%c", clear_color, value[i]);
    }
  }

  putchar('\n');
}

auto Test::expected_hex(View::Bytes value, Bool actual) -> void {
  write_label(actual);
  for (Count index = 0; index < value.get_size(); index++) {
    printf("%02X ", value[index]);
  }

  putchar('\n');
}

static auto output_break() -> void {
  printf(
      "%s[==============================================================]\n%s",
      dark_color, clear_color);
}

static auto output_results(Count test_count) -> void {
  printf("%s\n  Testing Completed:\n", perimortem_color);
  if (passed_tests) {
    printf(
        "%s      Passed:  %llu%s\n", pass_color,
        (unsigned long long)passed_tests, clear_color);
  }

  if (failed_tests) {
    printf(
        "%s      Failed:  %llu%s\n", fail_color,
        (unsigned long long)failed_tests, clear_color);
    for (Count index = 0; index < failed_tests; index++) {
      const auto& test = binary_tests[failed_test_indexes[index]];
      printf(
          "%s          %.*s:%llu%s\n", dark_color, (int)test.file.get_size(),
          test.file.get_data(), (unsigned long long)test.line, clear_color);
    }
  }

  if (not_run_tests) {
    printf(
        "%s     Not Run:  %llu%s\n", system_color,
        (unsigned long long)not_run_tests, clear_color);
  }

  R64 pass_rate =
      test_count == 0 ? 0.0 : R64(passed_tests) / R64(test_count) * 100.0;
  printf(
      "%s  Pass Rate: %s%llu / %llu%s ( %g %%)\n%s", perimortem_color,
      clear_color, (unsigned long long)passed_tests,
      (unsigned long long)test_count, system_color, pass_rate, clear_color);
}

int main(int argc, const char* argv[]) {
  S32 process_status = 0;
  if (Process::Fixture::dispatch(S32(argc), argv, process_status)) {
    return process_status;
  }

  test_suites = 0;
  passed_tests = 0;
  failed_tests = 0;

  Bool silent = argc == 2 && NullTerminated::to_view(argv[1]) == "silent"_view;
  if (argc > 2 || (argc == 2 && !silent)) {
    fprintf(stderr, "Usage: %s [silent]\n", argv[0]);
    return 1;
  }

  Count test_count = binary_tests_count;
  not_run_tests = test_count;

  Count longest_test_name = 12;

  const Harness* harness = nullptr;
  for (Count i = 0; i < test_count; i++) {
    const Instance& test = binary_tests[i];
    Count name_length = test.name.get_size();
    if (name_length > longest_test_name) {
      longest_test_name = name_length;
    }

    if (test.harness != harness) {
      test_suites++;
      harness = test.harness;
    }
  }

  harness = nullptr;

  if (!silent) {
    output_break();
    printf(
        "%s  Executing Perimortem test engine from Validation::Test\n",
        perimortem_color);
    printf(
        "  Tests found:  %s%llu%s (%llu Harness)%s\n", clear_color,
        (unsigned long long)test_count, system_color,
        (unsigned long long)test_suites, clear_color);
    output_break();
  }

  Time start_full = Time::now();
  for (Count i = 0; i < test_count; i++) {
    Count index = i;
    const Instance& test = binary_tests[index];
    if (test.harness == nullptr) {
      fprintf(stderr, "Registered test has no Harness.\n");
      return 1;
    }
    const Harness& selected_harness = *test.harness;
    if (&selected_harness != harness) {
      harness = &selected_harness;
      if (!silent) {
        printf(
            "%s[ START ] %.*s\n%s", dark_color,
            (int)selected_harness.name.get_size(),
            selected_harness.name.get_data(), clear_color);
      }
      selected_harness.init();
    }

    // Setup sink before setup() so tests can still override.
    Diagnostics::Log::set_sink(Test::capture_sink);
    captured_log_message = ""_view;

    selected_harness.setup();
    not_run_tests -= 1;

    Test::TestResult result = Test::TestResult::Pass;

    Time start = Time::now();
    test.func(result);
    R64 test_time_ms = start.measure().convert_to_milliseconds();

    selected_harness.teardown();
    Diagnostics::Log::set_sink(Diagnostics::Log::default_sink);
    switch (result) {
    case Test::TestResult::Pass:
      passed_tests += 1;
      if (!silent) {
        printf(
            "%s  [  PASS  ] %-*.*s", pass_color, (int)(longest_test_name + 2),
            (int)test.name.get_size(), Data::cast<char>(test.name.get_data()));
      }
      break;
    case Test::TestResult::Failed:
      failed_test_indexes[failed_tests] = index;
      failed_tests += 1;
      printf(
          "%s  [  FAIL  ] %-*.*s", fail_color, (int)(longest_test_name + 2),
          (int)test.name.get_size(), Data::cast<char>(test.name.get_data()));
      break;
    }

    if (!silent || result == Test::TestResult::Failed) {
      printf("%s  (%g ms)\n%s", system_color, test_time_ms, clear_color);
    }
  }

  R64 full_time_ms = start_full.measure().convert_to_milliseconds();

  if (!silent) {
    output_break();
  }
  output_results(test_count);
  printf(
      "%s\n  Total Time:  %s%g ms\n\n%s", perimortem_color, clear_color,
      full_time_ms, clear_color);
  if (!silent) {
    output_break();
    printf("\n");
  }
  fflush(stdout);
  return (int)failed_tests;
}
