// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Shader::Interpreter {

class Stage {
 public:
  Stage() = delete;

  static auto is_next(const Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Shader::Language::Program& program,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation) -> Bool;
};

}  // namespace Tetrodotoxin::Shader::Interpreter
