// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Language::Parser {

// Parses a dialect declreation and returns the Dialect name.
class Dialect {
 public:
  static auto parse(Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Tetrodotoxin::Language::Parser
