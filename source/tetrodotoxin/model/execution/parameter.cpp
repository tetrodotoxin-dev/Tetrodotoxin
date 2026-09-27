// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/model/execution/parameter.hpp"

auto tetrodotoxin_model_execution_parameter_representation()
    -> const ttx_representation* {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Tetrodotoxin::Model::Execution::Parameter>();
}
