// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Execution {

// Local reads one Block scoped addressable declaration. The resulting Local
// keeps optional Type and initializer edges directly and never retains a
// partially consumed grammar record.
class Local {
 public:
  Local() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Language::Flow::Block& host)
      -> Perimortem::Core::Option<Language::Flow::Local&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Execution
