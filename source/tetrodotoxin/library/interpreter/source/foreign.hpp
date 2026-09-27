// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/foreign.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Library::Interpreter::Source {

// Foreign reads one external ABI block into the Source owned Foreign context.
// A closing brace commits the complete block so an interrupted declaration
// cannot change an earlier retained ABI surface.
class Foreign {
 public:
  Foreign() = delete;

  static auto is_next(const Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Language::Foreign& host,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation) -> Bool;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Source
