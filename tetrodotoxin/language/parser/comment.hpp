// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Language::Parser {

// Comment consumes the consecutive authored comment prefix at the current
// cursor and materializes one compact TTX documentation block in that Cursor's
// source transaction Arena. Raw comments are consumed but do not become
// presentation lines. The retained line views continue to borrow source bytes.
//
// Empty Documentation means either that no comment was present or that the
// prefix contained only raw comments. The former leaves the Cursor unchanged,
// while the latter consumes the raw prefix. Callers decide whether the absence
// of presentation lines is legal at the selected grammar position.
class Comment {
 public:
  static auto parse(Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> const Tetrodotoxin::Source::Documentation&;
};

}  // namespace Tetrodotoxin::Language::Parser
