// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Language::Parser {

// Import parses the common Alias-producing source preamble shared by every
// installed Dialect.
class Import {
 public:
  static auto is_next(const Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation)
      -> Perimortem::Core::Option<Language::Import::Description>;

  // A Dialect relationship can consume the same source or Package Type
  // expression without manufacturing an authored Alias declaration. The
  // supplied local name remains private to that relationship owner.
  static auto parse_expression(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation,
      Perimortem::Core::View::Bytes name,
      Language::Visibility visibility,
      Tetrodotoxin::Source::Lexical::Token opening,
      Tetrodotoxin::Source::Lexical::Token name_token = {})
      -> Perimortem::Core::Option<Language::Import::Description>;
};

}  // namespace Tetrodotoxin::Language::Parser
