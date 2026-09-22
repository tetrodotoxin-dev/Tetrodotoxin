// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/render/language/layout.hpp"

namespace Tetrodotoxin::Render::Interpreter {

class Layout {
 public:
  Layout() = delete;

  static auto parse(Tetrodotoxin::Source::Lexical::Cursor& cursor, Bool parameters)
      -> Perimortem::Core::Option<Language::Layout&>;
};

}  // namespace Tetrodotoxin::Render::Interpreter
