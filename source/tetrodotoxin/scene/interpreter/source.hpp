// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/scene/language/monograph.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Scene::Interpreter {

// Source composes one Scene source from its real owners. Library receives the
// declarations and executable bodies it understands, while Scene retains only
// its Signals and lifecycle relationships around the same semantic objects.
class Source {
 public:
  Source() = delete;

  static auto parse(
      Language::Monograph& monograph,
      Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void;
};

}  // namespace Tetrodotoxin::Scene::Interpreter
