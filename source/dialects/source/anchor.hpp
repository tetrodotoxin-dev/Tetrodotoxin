// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/anchor.h"
#include "tetrodotoxin/dialects/source/span.hpp"

namespace Tetrodotoxin::Dialects::Source {

// Anchor retains the complete source range relevant to one fact and selects
// the Token a diagnostic should point at. The Token may be empty or outside
// the Span because the final presentation owner decides whether a caret is
// meaningful for the source snapshot it actually renders.
class Anchor {
 public:
  constexpr explicit Anchor(tetrodotoxin_source_anchor value)
      : token(Token(value.token)), span(Token(value.start), Token(value.end)) {}
  constexpr auto get_abi() const -> tetrodotoxin_source_anchor {
    return {
      token.get_abi(), span.get_start().get_abi(), span.get_end().get_abi()};
  }
  static constexpr auto create(Span span) -> Anchor {
    return Anchor(span.get_start(), span);
  }

  static constexpr auto create(Token token, Span span) -> Anchor {
    return Anchor(token, span);
  }

  static constexpr auto create(Token token, Span first, Span last) -> Anchor {
    if (!first) {
      return Anchor(token, last);
    }
    if (!last) {
      return Anchor(token, first);
    }

    Token start = first.get_start();
    if (last.get_offset() < first.get_offset()) {
      start = last.get_start();
    }

    Token end = first.get_end();
    Count first_extent = Count(end.get_offset()) + Count(end.get_size());
    Token last_end = last.get_end();
    Count last_extent =
        Count(last_end.get_offset()) + Count(last_end.get_size());
    if (last_extent > first_extent) {
      end = last_end;
    }

    return Anchor(token, Span(start, end));
  }

  constexpr auto get_token() const -> Token { return token; }

  constexpr auto get_span() const -> Span { return span; }

 private:
  constexpr Anchor(Token token, Span span) : token(token), span(span) {}

  Token token;
  Span span;
};

static_assert(sizeof(Anchor) == 24);

}  // namespace Tetrodotoxin::Dialects::Source

TTX_DATA_RECORD(
    tetrodotoxin_source_anchor,
    TTX_DATA_MEMBER(tetrodotoxin_source_anchor, token),
    TTX_DATA_MEMBER(tetrodotoxin_source_anchor, start),
    TTX_DATA_MEMBER(tetrodotoxin_source_anchor, end));
