// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/tokenization.hpp"

auto tetrodotoxin_source_tokenization_representation()
    -> const ttx_representation* {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Tetrodotoxin::Source::Tokenization>();
}
