// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Validation::Process {

struct Request {
  Perimortem::Core::View::Bytes executable;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> arguments;
  Perimortem::Core::View::Bytes standard_input;
  U64 timeout_nanoseconds = 1'000'000'000;
};

struct Observation {
  Bool launched = False;
  Bool timed_out = False;
  S32 exit_status = -1;
  Perimortem::Memory::Dynamic::Bytes standard_input;
  Perimortem::Memory::Dynamic::Bytes standard_output;
  Perimortem::Memory::Dynamic::Bytes standard_error;
  Perimortem::Memory::Dynamic::Bytes runner_error;
};

struct Expectation {
  Bool timed_out = False;
  S32 exit_status = 0;
  Perimortem::Core::View::Bytes standard_input;
  Perimortem::Core::View::Bytes standard_output;
  Perimortem::Core::View::Bytes standard_error;
};

enum class Difference {
  None,
  Launch,
  StandardInput,
  Timeout,
  StandardOutput,
  StandardError,
  ExitStatus,
};

auto run(const Request& request) -> Observation;
auto compare(const Observation& observation, const Expectation& expectation)
    -> Difference;

}  // namespace Validation::Process
