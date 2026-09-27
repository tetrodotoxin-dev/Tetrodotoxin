// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/lexical/code.hpp"
#include "tetrodotoxin/source/lexical/token.h"
#include "ttx/data/form/native.hpp"

namespace Tetrodotoxin::Source::Lexical {

// Token adds native construction and inspection to the copied C record.
// Keeping constructors here matters even though both types occupy eight
// bytes: MSVC returns a constructed C++ record through a hidden pointer,
// while the plain C record returns in a register.
class Token : public tetrodotoxin_source_token {
 public:
  constexpr Token() : tetrodotoxin_source_token(0) {}
  constexpr Token(tetrodotoxin_source_token token)
      : tetrodotoxin_source_token(token) {}
  constexpr Token(U64 locator, Code code)
      : tetrodotoxin_source_token(
            (locator << 8) | static_cast<U8>(code.get_type())) {}

  constexpr explicit operator bool() const { return bool(is_valid()); }
  constexpr auto operator==(Token other) const -> Bool {
    return value == other.value;
  }
  constexpr auto operator!=(Token other) const -> Bool {
    return value != other.value;
  }
  constexpr auto is_valid() const -> Bool {
    return get_code() != Code::Type::Terminal;
  }
  constexpr auto get_code() const -> Code {
    return Code(static_cast<Code::Type>(U8(value)));
  }
  // Only the supplying provider interprets this number. Construction requires
  // a locator within the 56 bit domain promised by this contract.
  constexpr auto get_locator() const -> U64 { return value >> 8; }
};

static_assert(sizeof(Token) == 8);

}  // namespace Tetrodotoxin::Source::Lexical

TTX_DATA_RECORD(
    tetrodotoxin_source_token,
    TTX_DATA_MEMBER(tetrodotoxin_source_token, value));
