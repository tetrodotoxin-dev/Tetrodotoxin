// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/app/language/scene.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::App::Interpreter {

// Scene reads one initial Scene and the ordered Signal mappings that govern the
// live application stack.
class Scene {
 public:
  Scene() = delete;

  static auto is_next(const Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation)
      -> Perimortem::Core::Option<Language::Scene&>;
};

}  // namespace Tetrodotoxin::App::Interpreter
