// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/type_reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Language::Parser {

class TypeReference {
 public:
  TypeReference() = delete;

  static auto parse(Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::TypeReference>;
};

}  // namespace Tetrodotoxin::Language::Parser
