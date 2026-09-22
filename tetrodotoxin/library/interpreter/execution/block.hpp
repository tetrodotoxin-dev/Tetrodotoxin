// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/interpreter/execution/statement_parser.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Execution {

// Block interprets ordered Statement membership for both braced and compact
// source forms. Every entry retains its real semantic owner while Block keeps
// only source order and lexical lifetime.
class Block {
 public:
  Block() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Language::Model::Callable& function,
      const Language::Model::Type& access_scope,
      Perimortem::Core::Option<Tetrodotoxin::Source::Reference<
          const Tetrodotoxin::Source::Abstract>> enclosing_loop = {},
      Perimortem::Core::Option<const StatementParser&> extension = {})
      -> Perimortem::Core::Option<Language::Flow::Block&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Execution
