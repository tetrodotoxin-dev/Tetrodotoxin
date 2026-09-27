// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/field.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Execution {

// A field is one typed value position. Its Type edge retains the supplying
// policy rather than recovering a native Type object. Names and source spans
// belong to a declaration policy and are not required to use this position.
class Field {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_EXECUTION_FIELD_ID_HIGH,
    TETRODOTOXIN_MODEL_EXECUTION_FIELD_ID_LOW,
  };
  using Api = tetrodotoxin_model_execution_field;

  explicit constexpr Field(Api api) : api(api) {}

  auto get_type() const -> Ttx::Concept::Abstract {
    return Ttx::Concept::Abstract(api.get_type(api.source));
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Model::Execution

TTX_DATA_RECORD(
    tetrodotoxin_model_execution_field,
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_field, source),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_field, get_type));
