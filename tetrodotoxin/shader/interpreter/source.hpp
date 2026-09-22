// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/shader/language/monograph.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Shader::Interpreter {

class Source {
 public:
  Source() = delete;

  static auto parse(
      Language::Monograph& monograph,
      Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void;
};

}  // namespace Tetrodotoxin::Shader::Interpreter
