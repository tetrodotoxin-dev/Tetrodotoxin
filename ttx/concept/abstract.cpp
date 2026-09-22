// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/abstract.hpp"

auto ttx_abstract_representation() -> const ttx_representation* {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Ttx::Concept::Abstract>();
}
