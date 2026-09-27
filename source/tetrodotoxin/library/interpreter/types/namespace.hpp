// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/interpreter/parsed.hpp"
#include "tetrodotoxin/library/language/types/namespace.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Types {

class Namespace {
 public:
  Namespace() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Parsed<Language::Types::Namespace>>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Types
