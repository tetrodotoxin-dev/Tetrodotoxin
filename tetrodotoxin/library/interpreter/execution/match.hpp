// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/interpreter/execution/statement_parser.hpp"
#include "tetrodotoxin/library/language/flow/match.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Execution {

// Match reads constant, payload, and final discard cases into one semantic
// Match. Each nested Block receives the exact lexical context selected by its
// case rather than a reconstructed pattern scope.
class Match {
 public:
  Match() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Language::Flow::Block& lexical_context,
      Language::Model::Callable& function,
      const Language::Model::Type& access_scope,
      Perimortem::Core::Option<const StatementParser&> extension)
      -> Perimortem::Core::Option<Language::Flow::Match&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Execution
