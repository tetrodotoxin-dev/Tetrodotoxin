// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/model/execution/field.hpp"

auto tetrodotoxin_model_execution_field_representation()
    -> const ttx_representation* {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Tetrodotoxin::Model::Execution::Field>();
}
