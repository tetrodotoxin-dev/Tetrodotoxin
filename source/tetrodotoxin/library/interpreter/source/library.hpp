// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Source {

// Library interprets one complete Library source body into its synthetic
// Source model. Each recognized declaration enters that real context while
// recovery keeps later source available to editor queries.
class Library {
 public:
  Library() = delete;

  static auto parse(
      Language::Types::Source& source,
      Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Source
