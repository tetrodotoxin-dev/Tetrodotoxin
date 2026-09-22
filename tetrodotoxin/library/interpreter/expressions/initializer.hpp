// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Expressions {

// Initializer reads one explicit `new` expression and hands its target route
// and argument Pack to the retained Initializer. The model keeps evaluation
// order without importing the grammar that produced it.
class Initializer {
 public:
  Initializer() = delete;

  static auto is_next(const Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      const Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::Expressions::Initializer&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Expressions
