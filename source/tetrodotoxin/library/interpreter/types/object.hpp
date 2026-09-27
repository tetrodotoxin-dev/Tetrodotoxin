// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/interpreter/parsed.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Types {

// Object validates the managed Type declaration and uses the same member body
// grammar as Structure. The resulting semantic identity remains an Object and
// carries no interpretation wrapper into its host Dialect.
class Object {
 public:
  Object() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Parsed<Language::Types::Object>>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Types
