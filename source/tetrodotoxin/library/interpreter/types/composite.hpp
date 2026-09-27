// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/interpreter/parsed.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Types {

// Composite reads the shared member body used by Structure and Object. The
// concrete Type exists before this entry so every nested declaration can keep
// its final host identity while the body is still being interpreted.
class Composite {
 public:
  Composite() = delete;

  static auto parse_body(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Language::Types::Composite& structure,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Source::Lexical::Token kind_token) -> ParseState;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Types
