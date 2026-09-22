// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

namespace Validation {

// Package semantic tests exercise the real source envelope and Dialect without
// asking Workspace to publish an intentionally incomplete Package. Proxying the
// source first keeps every Documentation line, route, and statement Span in the
// same Arena as the Monograph that retains them.
inline auto interpret_package(
    Perimortem::Memory::Allocator::Arena& arena,
    Tetrodotoxin::Package::Dialect& dialect,
    Tetrodotoxin::Source::Lexical::Errors& errors,
    Perimortem::Core::View::Bytes source,
    Perimortem::Core::View::Bytes path)
    -> Perimortem::Core::Option<Tetrodotoxin::Package::Language::Monograph&> {
  Perimortem::Core::View::Bytes retained_source = arena.proxy(source);
  Perimortem::Core::View::Bytes retained_path = arena.proxy(path);
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(arena, retained_source, retained_path);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Tetrodotoxin::Source::Lexical::Cursor cursor(tokenizer, errors, associations);
  Count error_count = errors.get_size();
  const auto opening = cursor.current();
  const auto& documentation =
      Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  const auto declaration = cursor.current();
  const auto name = Tetrodotoxin::Language::Parser::Dialect::parse(cursor);
  if (name != dialect.get_name()) {
    return {};
  }
  const auto anchor = Tetrodotoxin::Source::Lexical::Anchor::create(
      declaration, Tetrodotoxin::Source::Lexical::Span(opening, cursor.peek(-1)));
  auto interpreted = dialect.interpret(cursor, documentation, anchor, dialect);
  if (!interpreted || errors.get_size() != error_count ||
      !interpreted->is<Tetrodotoxin::Package::Language::Monograph>()) {
    return {};
  }

  return static_cast<Tetrodotoxin::Package::Language::Monograph&>(*interpreted);
}

}  // namespace Validation
