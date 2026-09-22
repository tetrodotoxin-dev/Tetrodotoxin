// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <stdio.h>

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::System {

// Reads and writes complete lines through borrowed C streams. The default
// constructor selects stdin and stdout. The explicit constructor accepts other
// streams whose lifetime remains with the caller.
//
// read_line consumes bytes from the input stream one at a time. Its reads block
// until LF, EOF, or a stream failure lets the operation complete. The result
// excludes LF, removes one CR directly before LF, and preserves every other
// delivered byte. A blank line is an engaged empty value while immediate EOF
// or read failure is absent.
//
// Terminal performs no input editing or backtracking. An interactive terminal
// may process erase keys through its line discipline before stdin yields the
// completed input. Any backspace byte delivered by another stream remains in
// the returned bytes.
//
// write_line writes the supplied bytes, appends one LF, and flushes the output.
// It returns false when the content, terminator, or flush cannot complete.
class Terminal {
 public:
  Terminal();
  Terminal(FILE& input, FILE& output);

  auto read_line() -> Core::Option<Memory::Dynamic::Bytes>;
  auto write_line(Core::View::Bytes data) -> Bool;

 private:
  FILE& input;
  FILE& output;
};

}  // namespace Perimortem::System
