// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/shader/language/monograph.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Shader::Interpreter {

class Bridge {
 public:
  Bridge() = delete;

  static auto parse(
      Language::Monograph& monograph,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition) -> Bool;
};

}  // namespace Tetrodotoxin::Shader::Interpreter
