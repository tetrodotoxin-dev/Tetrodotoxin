// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/token.hpp"
#include "tetrodotoxin/dialects/source/tokens.h"
#include "ttx/concept/abstract.hpp"

TTX_DATA_RECORD(
    tetrodotoxin_source_tokens,
    TTX_DATA_MEMBER(tetrodotoxin_source_tokens, representation),
    TTX_DATA_MEMBER(tetrodotoxin_source_tokens, data),
    TTX_DATA_MEMBER(tetrodotoxin_source_tokens, size),
    TTX_DATA_MEMBER(tetrodotoxin_source_tokens, count));

namespace Tetrodotoxin::Dialects::Source {

// Tokens admits one described compact buffer. Indexed reads copy the public
// record so foreign storage needs no native alignment or C++ object lifetime.
// Negotiation stays outside the per token path. An unsupported representation
// gives an invalid reader rather than guessing the provider's record stride.
class Tokens {
 public:
  explicit Tokens(tetrodotoxin_source_tokens value) : value(value) {
    const auto& expected = Ttx::Data::Form::Compiled<Ttx::Data::Form::Native<
        tetrodotoxin_source_token>::reference>::get_representation();
    valid = value.representation &&
            expected.compatible(*value.representation) && value.data &&
            value.count &&
            value.count <= value.size / sizeof(tetrodotoxin_source_token);
    if (valid) {
      const auto terminal = (*this)[value.count - 1];
      valid =
          terminal.get_code() == Code::Type::Terminal && !terminal.get_size();
    }
  }
  auto is_valid() const -> Bool { return valid; }
  auto get_size() const -> Count { return valid ? value.count : 0; }
  auto get_abi() const -> tetrodotoxin_source_tokens { return value; }
  auto operator[](Count index) const -> Token {
    if (!valid || index >= value.count) {
      return Token();
    }
    tetrodotoxin_source_token token;
    Perimortem::Core::Data::copy(
        reinterpret_cast<U8*>(&token), value.data + index * sizeof(token),
        sizeof(token));
    return Token(token);
  }

 private:
  tetrodotoxin_source_tokens value;
  Bool valid = false;
};
}  // namespace Tetrodotoxin::Dialects::Source
