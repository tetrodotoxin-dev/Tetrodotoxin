// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

namespace Perimortem::System {

// Parses typical CLI arguments into a Perimortem friendly data structure.
// Supports named keys, positionals (in a sense), and repeat keys.
//
// Only valid keys are let through with `help` being a canonical key that is
// always valid. Further input validation is left up to the caller to allow
// for flexability.
//
// `log_help` can be used to output structured help text based on a given config
// to `Diagnostics` using `Level::Info`.
class Args {
 public:
  // Values are parsed as a map of valid keys with a vector of their results.
  // It's up to the calling program to decide if missing keys are an issue or
  // if multiple keys cause an error.
  using Values = Memory::Managed::
      Map<Core::View::Bytes, Memory::Managed::Vector<Core::View::Bytes>*>;

  // Takes a map of named variable keys and help text as values.
  // A key of "" (empty view) is used if the tool wants to support positionals.
  static auto parse(
      Memory::Allocator::Arena& arena,
      const Memory::Managed::Map<Core::View::Bytes, Core::View::Bytes>& config,
      Core::View::Vector<Core::View::Bytes> arguments) -> Values;

  static auto log_help(
      Memory::Allocator::Arena& arena,
      Core::View::Bytes tool_summary,
      const Memory::Managed::Map<Core::View::Bytes, Core::View::Bytes>& config,
      Core::View::Vector<Core::View::Bytes> arguments) -> void;
};

}  // namespace Perimortem::System
