// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/render/language/monograph.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Render::Interpreter {

// Source reads the ordered declarations of one Render Monograph. Concrete
// declaration interpreters create the semantic owners directly in that source
// transaction and Source only coordinates recovery between them.
class Source {
 public:
  Source() = delete;

  static auto parse(
      Language::Monograph& monograph,
      Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void;
};

}  // namespace Tetrodotoxin::Render::Interpreter
