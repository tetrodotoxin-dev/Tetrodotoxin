// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/dialects/source/token.hpp"

namespace Tetrodotoxin::Dialects::Source {

// Span is a compact pair of Tokens delimiting one contiguous source range.
//
// A default Span is invalid. A valid Span may begin and end with the same
// Token, but construction always places the earlier Token first.
class Span {
 public:
  constexpr Span() = default;
  constexpr explicit Span(Token token) : start(token), end(token) {}
  constexpr Span(Token start, Token end)
      : start(start.get_offset() <= end.get_offset() ? start : end),
        end(start.get_offset() <= end.get_offset() ? end : start) {}

  constexpr operator bool() const { return bool(is_valid()); }

  constexpr auto caculate_text(Perimortem::Core::View::Bytes source) const
      -> Perimortem::Core::View::Bytes {
    return source.slice(get_offset(), get_size());
  }

  // A span is valid if its start is valid as end can be a Terminal node.
  constexpr auto is_valid() const -> Bool { return start.is_valid(); }

  constexpr auto get_offset() const -> U16 { return start.get_offset(); }
  constexpr auto get_line() const -> U16 { return start.get_line(); }
  constexpr auto get_line_count() const -> U16 {
    return end.get_line() - start.get_line() + 1;
  }
  constexpr auto get_column() const -> U16 { return start.get_column(); }
  constexpr auto get_size() const -> Count {
    return Count(end.get_offset()) - Count(start.get_offset()) +
           Count(end.get_size());
  }

  constexpr auto get_start() const -> Token { return start; }
  constexpr auto get_end() const -> Token { return end; }

 private:
  Token start;
  Token end;
};

static_assert(sizeof(Span) == 16);

}  // namespace Tetrodotoxin::Dialects::Source
