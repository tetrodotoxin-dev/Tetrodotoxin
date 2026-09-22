// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/interpreter/parsed.hpp"
#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Declarations {

// Alias reads the authored redirection while the model keeps only its stable
// identity and delayed target edge. Resolution remains an Alias operation and
// never asks this parser to revisit source.
class Alias {
 public:
  Alias() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Parsed<Language::Alias>>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Declarations
