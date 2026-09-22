// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/interpreter/execution/statement_parser.hpp"
#include "tetrodotoxin/library/language/flow/branch.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Execution {

// Branch reads one `if` or `while` statement together with its exact nested
// Blocks. Loop ownership is established while lexical nesting is visible and
// remains a real graph edge afterward.
class Branch {
 public:
  Branch() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Language::Flow::Block& lexical_context,
      Language::Model::Callable& function,
      const Language::Model::Type& access_scope,
      Perimortem::Core::Option<const StatementParser&> extension)
      -> Perimortem::Core::Option<Language::Flow::Branch&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Execution
