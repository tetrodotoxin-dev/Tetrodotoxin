// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// Literal consumes one complete scalar or byte operand and constructs its real
// Constant in the graph Arena. Library's binary wide scalar addresses supply
// inferred Types while the source context remains the sole owner of Embedded
// resolution. Contextual fitting begins only after the complete Expression has
// synthesized its Type, so Literal accepts no target Type. Embedded Constants
// borrow Resource bytes, requiring the supplied domain not to outlive the
// Resource dependency domain.
class Literal {
 public:
  Literal() = delete;

  static auto parse(
      const Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::Constant&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter
