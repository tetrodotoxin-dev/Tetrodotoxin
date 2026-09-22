// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/render/language/monograph.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Render::Interpreter {

class Declaration {
 public:
  Declaration() = delete;

  static auto parse(
      Tetrodotoxin::Source::Abstract& host,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation) -> Bool;
};

}  // namespace Tetrodotoxin::Render::Interpreter
