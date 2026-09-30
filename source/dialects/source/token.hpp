// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "tetrodotoxin/dialects/source/code.hpp"
#include "tetrodotoxin/dialects/source/token.h"
#include "ttx/data/form/native.hpp"
#include "ttx/data/form/representation.hpp"

namespace Tetrodotoxin::Dialects::Source {

// Token is the compact source coordinate carried by one frontend Code. Its byte
// offset, line, column, and size keep authored evidence beside the
// classification without borrowing another position table. The eight byte form
// matches the bounded source transactions supported by the current Cursor
// contract.
class Token {
 public:
  constexpr Token() = default;
  constexpr explicit Token(tetrodotoxin_source_token value)
      : offset(value.offset),
        line(value.line),
        column(value.column),
        size(value.size),
        code(static_cast<Code::Type>(value.code)) {}
  constexpr auto get_abi() const -> tetrodotoxin_source_token {
    static_assert(__builtin_offsetof(Token, offset) == 0);
    static_assert(__builtin_offsetof(Token, line) == 2);
    static_assert(__builtin_offsetof(Token, column) == 4);
    static_assert(__builtin_offsetof(Token, size) == 6);
    static_assert(__builtin_offsetof(Token, code) == 7);
    return {offset, line, column, size, static_cast<U8>(code.get_type())};
  }
  constexpr Token(U16 offset, U16 line, U16 column, U8 size, Code code)
      : offset(offset), line(line), column(column), size(size), code(code) {}

  constexpr operator bool() const { return bool(is_valid()); }

  constexpr auto caculate_text(Perimortem::Core::View::Bytes source) const
      -> Perimortem::Core::View::Bytes {
    return source.slice(get_offset(), get_size());
  }

  constexpr auto is_valid() const -> Bool {
    return code != Code::Type::Terminal;
  }

  constexpr auto get_offset() const -> U16 { return offset; }
  constexpr auto get_line() const -> U16 { return line; }
  constexpr auto get_column() const -> U16 { return column; }
  constexpr auto get_size() const -> U8 { return size; }
  constexpr auto get_code() const -> Code { return code; }

 private:
  U16 offset = 0;
  U16 line = 0;
  U16 column = 0;
  U8 size = 0;
  Code code = Code::Type::Terminal;
};

static_assert(sizeof(Token) == 8);

}  // namespace Tetrodotoxin::Dialects::Source

TTX_DATA_RECORD(
    tetrodotoxin_source_token,
    TTX_DATA_MEMBER(tetrodotoxin_source_token, offset),
    TTX_DATA_MEMBER(tetrodotoxin_source_token, line),
    TTX_DATA_MEMBER(tetrodotoxin_source_token, column),
    TTX_DATA_MEMBER(tetrodotoxin_source_token, size),
    TTX_DATA_MEMBER(tetrodotoxin_source_token, code));
