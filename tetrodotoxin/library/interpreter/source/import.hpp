// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Library::Interpreter::Source {

// Import reads one complete `using` statement. The retained Import receives
// its exact route and source range while recovery remains local to this
// grammar entry.
class Import {
 public:
  Import() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation)
      -> Perimortem::Core::Option<Language::Import>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Source
