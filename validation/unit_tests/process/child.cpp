// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/process/child.hpp"

#include "validation/process/fixture.hpp"
#include "validation/unit_test.hpp"

#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static constexpr View::Bytes child_switch = "--validation-process-child"_view;
static constexpr View::Bytes request_bytes = "request\n"_view;
static constexpr View::Bytes response_bytes = "response\n"_view;
static constexpr View::Bytes diagnostic_bytes = "diagnostic\n"_view;

static auto write_all(S32 descriptor, View::Bytes bytes) -> Bool {
  Count offset = 0;
  while (offset < bytes.get_size()) {
    ssize_t count =
        write(descriptor, bytes.get_data() + offset, bytes.get_size() - offset);
    if (count > 0) {
      offset += Count(count);
      continue;
    }

    if (count < 0 && errno == EINTR) {
      continue;
    }

    return False;
  }

  return True;
}

static auto read_all(S32 descriptor) -> Dynamic::Bytes {
  Dynamic::Bytes bytes;
  Static::Vector<U8, 4096> buffer;
  while (true) {
    ssize_t count = read(descriptor, buffer.get_data(), buffer.get_size());
    if (count > 0) {
      bytes.concat(View::Bytes(buffer.get_data(), Count(count)));
      continue;
    }

    if (count < 0 && errno == EINTR) {
      continue;
    }

    return bytes;
  }
}

static auto run_child(View::Bytes mode) -> S32 {
  if (mode == "contract"_view) {
    Dynamic::Bytes input = read_all(STDIN_FILENO);
    if (!(input == request_bytes)) {
      write_all(STDERR_FILENO, "unexpected input\n"_view);
      return 2;
    }

    write_all(STDOUT_FILENO, response_bytes);
    write_all(STDERR_FILENO, diagnostic_bytes);
    return 7;
  }

  if (mode == "timeout"_view) {
    while (true) {
      pause();
    }
  }

  write_all(STDERR_FILENO, "unknown process fixture mode\n"_view);
  return 3;
}

auto Process::Fixture::dispatch(
    S32 argument_count,
    const char* const arguments[],
    S32& status) -> Bool {
  if (argument_count < 2 ||
      NullTerminated::to_view(arguments[1]) != child_switch) {
    return False;
  }

  if (argument_count != 3) {
    status = 3;
    return True;
  }

  status = run_child(NullTerminated::to_view(arguments[2]));
  return True;
}

static auto executable_path() -> Dynamic::Bytes {
  Static::Vector<U8, 4096> path;
  ssize_t size = readlink(
      "/proc/self/exe", Data::cast<char>(path.get_data()), path.get_size());
  if (size <= 0 || Count(size) == path.get_size()) {
    return Dynamic::Bytes();
  }

  return Dynamic::Bytes(View::Bytes(path.get_data(), Count(size)));
}

static auto observe(
    View::Bytes executable,
    View::Bytes mode,
    View::Bytes input = View::Bytes(),
    U64 timeout_nanoseconds = 1'000'000'000) -> Process::Observation {
  Static::Vector<View::Bytes, 2> arguments = {{child_switch, mode}};
  Process::Request request = {
    .executable = executable,
    .arguments = arguments,
    .standard_input = input,
    .timeout_nanoseconds = timeout_nanoseconds,
  };
  return Process::run(request);
}

static Harness ProcessTests = {
  .name = "Validation::Process"_view,
};

PERIMORTEM_UNIT_TEST(ProcessTests, process_contract) {
  Dynamic::Bytes executable = executable_path();
  ASSERT_NOT(executable.is_empty());

  Process::Expectation expectation = {
    .exit_status = 7,
    .standard_input = request_bytes,
    .standard_output = response_bytes,
    .standard_error = diagnostic_bytes,
  };
  Process::Observation observation =
      observe(executable, "contract"_view, request_bytes);

  EXPECT(observation.launched);
  EXPECT_NOT(observation.timed_out);
  EXPECT(observation.runner_error.is_empty());
  EXPECT(
      Process::compare(observation, expectation) == Process::Difference::None);
}

PERIMORTEM_UNIT_TEST(ProcessTests, deadline_termination) {
  Dynamic::Bytes executable = executable_path();
  ASSERT_NOT(executable.is_empty());

  Process::Observation observation =
      observe(executable, "timeout"_view, View::Bytes(), 100'000'000);
  Process::Expectation completed;

  EXPECT(observation.launched);
  EXPECT(observation.timed_out);
  EXPECT(observation.runner_error.is_empty());
  EXPECT_EQ(observation.exit_status, S32(128 + SIGKILL));
  EXPECT(
      Process::compare(observation, completed) == Process::Difference::Timeout);
}

PERIMORTEM_UNIT_TEST(ProcessTests, result_differences) {
  Process::Observation observation = {
    .launched = True,
    .exit_status = 7,
  };
  observation.standard_input = request_bytes;
  observation.standard_output = response_bytes;
  observation.standard_error = diagnostic_bytes;
  Process::Expectation expectation = {
    .exit_status = 7,
    .standard_input = request_bytes,
    .standard_output = response_bytes,
    .standard_error = diagnostic_bytes,
  };

  EXPECT(
      Process::compare(observation, expectation) == Process::Difference::None);

  observation.launched = False;
  EXPECT(
      Process::compare(observation, expectation) ==
      Process::Difference::Launch);
  observation.launched = True;

  observation.standard_input = "different"_view;
  EXPECT(
      Process::compare(observation, expectation) ==
      Process::Difference::StandardInput);
  observation.standard_input = request_bytes;

  observation.timed_out = True;
  EXPECT(
      Process::compare(observation, expectation) ==
      Process::Difference::Timeout);
  observation.timed_out = False;

  observation.standard_output = "different"_view;
  EXPECT(
      Process::compare(observation, expectation) ==
      Process::Difference::StandardOutput);
  observation.standard_output = response_bytes;

  observation.standard_error = "different"_view;
  EXPECT(
      Process::compare(observation, expectation) ==
      Process::Difference::StandardError);
  observation.standard_error = diagnostic_bytes;

  observation.exit_status = 8;
  EXPECT(
      Process::compare(observation, expectation) ==
      Process::Difference::ExitStatus);

  observation.exit_status = 7;
  observation.runner_error = "runner failure"_view;
  EXPECT(
      Process::compare(observation, expectation) ==
      Process::Difference::Launch);
}
