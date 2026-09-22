// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/flow/loop_control.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Execution {

// LoopControl binds authored `break` and `continue` directly to the nearest
// loop identity supplied by the current Block.
class LoopControl {
 public:
  LoopControl() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Language::Flow::Block& lexical_context)
      -> Perimortem::Core::Option<Language::Flow::LoopControl&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Execution
