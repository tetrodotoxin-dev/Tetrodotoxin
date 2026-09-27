// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/interpreter/parsed.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Types {

// Structure validates the authored Type prefix and then delegates the shared
// member body to Composite while retaining one concrete Structure identity.
class Structure {
 public:
  Structure() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Parsed<Language::Types::Structure>>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Types
