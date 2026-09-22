// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// TypeReference reads one delayed authored Type route. The retained value owns
// only source evidence and semantic arguments so later resolution never needs
// to recover parser state from its Anchor.
class TypeReference {
 public:
  TypeReference() = delete;

  static auto parse(
      const Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::TypeReference>;

  static auto parse_route(Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::TypeReference>;
};

}  // namespace Tetrodotoxin::Library::Interpreter
