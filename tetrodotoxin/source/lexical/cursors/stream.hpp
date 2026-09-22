// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/errors.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/lexical/stream.hpp"

namespace Tetrodotoxin::Source::Lexical::Cursors {

// The native provider borrows one immutable Stream and one Errors collector.
// Each acquired Cursor starts at index zero and owns its subsequent position.
// The receiver only supplies indexed observations and shared diagnostics, so
// forks need neither a new provider object nor another binding.
class Stream {
 public:
  constexpr Stream(
      const Lexical::Stream& stream,
      Tetrodotoxin::Source::Errors& errors)
      : stream(stream), errors(errors) {}
  auto get_interface() const -> Cursor;

 private:
  const Lexical::Stream& stream;
  Tetrodotoxin::Source::Errors& errors;
};

}  // namespace Tetrodotoxin::Source::Lexical::Cursors
