// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/lexical/span.h"
#include "tetrodotoxin/source/lexical/token.hpp"

namespace Tetrodotoxin::Source::Lexical {

// Span keeps the parser's endpoint operations beside Token's C++ facade.
// Cursor callbacks exchange the plain C record and leave interpretation of
// both locators with the provider that supplied them.
class Span : public tetrodotoxin_source_span {
 public:
  constexpr Span() : tetrodotoxin_source_span(Token(), Token()) {}
  constexpr Span(tetrodotoxin_source_span span)
      : tetrodotoxin_source_span(span) {}
  constexpr explicit Span(Token token)
      : tetrodotoxin_source_span(token, token) {}
  constexpr Span(Token start, Token end)
      : tetrodotoxin_source_span(start, end) {}
  constexpr auto get_start() const -> Token { return start; }
  constexpr auto get_end() const -> Token { return end; }
};

static_assert(sizeof(Span) == 16);

}  // namespace Tetrodotoxin::Source::Lexical

TTX_DATA_RECORD(
    tetrodotoxin_source_span,
    TTX_DATA_MEMBER(tetrodotoxin_source_span, start),
    TTX_DATA_MEMBER(tetrodotoxin_source_span, end));
