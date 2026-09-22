// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/scene/language/monograph.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Scene::Interpreter {

// Signal reads the compact event declaration owned by Scene and retains its
// optional payload as a delayed Library Type edge.
class Signal {
 public:
  Signal() = delete;

  static auto is_next(const Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Language::Monograph& monograph,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation) -> Bool;
};

}  // namespace Tetrodotoxin::Scene::Interpreter
