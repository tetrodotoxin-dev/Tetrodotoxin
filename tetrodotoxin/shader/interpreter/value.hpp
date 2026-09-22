// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Shader::Interpreter {

class Value {
 public:
  Value() = delete;

  static auto matches(
      const Tetrodotoxin::Language::Definition& definition,
      const Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Shader::Language::Program& program,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition) -> Bool;
};

}  // namespace Tetrodotoxin::Shader::Interpreter
