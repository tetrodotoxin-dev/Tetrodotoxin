// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Shader::Interpreter {

// Uniform reads one authored parameter into the Program owned Parameters
// Structure. The resulting Library Field remains the only semantic identity
// used by Stage bodies, CPU mutation, defaults, Archive restoration, and target
// packing.
class Uniform {
 public:
  Uniform() = delete;

  static auto parse(
      Shader::Language::Program& program,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition) -> Bool;
};

}  // namespace Tetrodotoxin::Shader::Interpreter
