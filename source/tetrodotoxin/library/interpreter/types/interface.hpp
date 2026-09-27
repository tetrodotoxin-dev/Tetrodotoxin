// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/interpreter/parsed.hpp"
#include "tetrodotoxin/library/language/types/interface.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Types {

class Interface {
 public:
  Interface() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Parsed<Language::Types::Interface>>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Types
