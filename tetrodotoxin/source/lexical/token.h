// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_LEXICAL_TOKEN_H
#define TETRODOTOXIN_SOURCE_LEXICAL_TOKEN_H

#include "perimortem/core/perimortem.h"

#ifdef __cplusplus
#include "tetrodotoxin/source/lexical/code.hpp"
#endif

#define TETRODOTOXIN_TOKEN_TERMINAL 0
#define TETRODOTOXIN_TOKEN_UNKNOWN 255
#define TETRODOTOXIN_TOKEN_LOCATOR_MAX 0x00ffffffffffffffULL

// A Token is a copied value used with the Cursor that supplied it. Its low
// eight bits are the Code, with 0 ending the stream and 255 marking unknown
// input. The upper 56 bits are a provider local locator. Consumers inspect the
// Code and return the Token to Cursor for spelling and source coordinates.
//
// Keeping coordinates behind Cursor makes long text and large source offsets
// independent of this eight byte carrier. The locator is neither an address
// nor a globally meaningful identity. Copying a Token retains no storage.
typedef struct tetrodotoxin_source_token {
  U64 value;

#ifdef __cplusplus
  using Code = Tetrodotoxin::Source::Lexical::Code;

  constexpr tetrodotoxin_source_token() : value(0) {}
  constexpr tetrodotoxin_source_token(U64 locator, Code code)
      : value((locator << 8) | static_cast<U8>(code.get_type())) {}

  constexpr explicit operator bool() const { return bool(is_valid()); }
  constexpr auto operator==(tetrodotoxin_source_token other) const -> Bool {
    return value == other.value;
  }
  constexpr auto operator!=(tetrodotoxin_source_token other) const -> Bool {
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
#endif
} tetrodotoxin_source_token;

#endif
