// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/model/execution/layout.hpp"

auto tetrodotoxin_model_execution_layout_representation()
    -> const ttx_representation* {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Tetrodotoxin::Model::Execution::Layout>();
}
