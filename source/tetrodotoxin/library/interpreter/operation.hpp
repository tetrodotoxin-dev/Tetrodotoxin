// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// Operation maps one authored operator Token to the concrete semantic owner
// that retains its operands. Precedence belongs to Expression while this entry
// owns the grammar shared by each unary or binary family.
class Operation {
 public:
  Operation() = delete;

  static auto parse_binary(
      Tetrodotoxin::Source::Lexical::Code::Type code,
      const Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Language::Model::Pack& left,
      Tetrodotoxin::Source::Lexical::Span left_span)
      -> Perimortem::Core::Option<Language::Expression&>;

  static auto parse_prefix(
      Tetrodotoxin::Source::Lexical::Code::Type code,
      const Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::Expression&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter
