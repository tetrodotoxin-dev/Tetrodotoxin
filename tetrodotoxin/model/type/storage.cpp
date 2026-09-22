// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/model/type/storage.hpp"

auto tetrodotoxin_model_type_storage_representation()
    -> const ttx_representation* {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Tetrodotoxin::Model::Type::Storage>();
}
