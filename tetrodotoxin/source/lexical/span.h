// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_LEXICAL_SPAN_H
#define TETRODOTOXIN_SOURCE_LEXICAL_SPAN_H

#include "tetrodotoxin/source/lexical/token.h"

// A parser remembers the first and last Token of a production. The provider
// resolves their source range, because their locators need not be byte offsets
// or numerically ordered. Both endpoints belong to the same Cursor input.
typedef struct tetrodotoxin_source_span {
  tetrodotoxin_source_token start;
  tetrodotoxin_source_token end;

#ifdef __cplusplus
  using Token = tetrodotoxin_source_token;
  constexpr tetrodotoxin_source_span() : start(), end() {}
  constexpr explicit tetrodotoxin_source_span(Token token) : start(token), end(token) {}
  constexpr tetrodotoxin_source_span(Token start, Token end) : start(start), end(end) {}
  constexpr auto get_start() const -> Token { return start; }
  constexpr auto get_end() const -> Token { return end; }
#endif
} tetrodotoxin_source_span;

#endif
