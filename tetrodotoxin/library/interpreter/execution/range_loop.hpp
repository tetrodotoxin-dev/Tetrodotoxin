// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/interpreter/execution/statement_parser.hpp"
#include "tetrodotoxin/library/language/flow/range_loop.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Execution {

// RangeLoop reads the named binding Layout, input Pack, and body while the
// lexical scope is available. The retained loop keeps those exact semantic
// edges and resolves each delayed binding Type later.
class RangeLoop {
 public:
  RangeLoop() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Language::Flow::Block& lexical_context,
      Language::Model::Callable& function,
      const Language::Model::Type& access_scope,
      Perimortem::Core::Option<const StatementParser&> extension)
      -> Perimortem::Core::Option<Language::Flow::RangeLoop&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Execution
