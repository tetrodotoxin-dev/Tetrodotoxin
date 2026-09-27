// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/interpreter/parsed.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Declarations {

// Field reads one Library addressable declaration and gives the retained
// model every semantic edge established by that authored form. A failed edge
// remains absent on the real Field rather than surviving as parser state.
class Field {
 public:
  Field() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Parsed<Language::Field>>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Declarations
