// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/parameter.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Execution {

// A parameter use refers to the exact declared field of its function. Keeping
// that edge lets a terminal connect a use to its input without reading names
// or assuming a native parameter class. The field retains its original policy.
class Parameter {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_EXECUTION_PARAMETER_ID_HIGH,
    TETRODOTOXIN_MODEL_EXECUTION_PARAMETER_ID_LOW,
  };
  using Api = tetrodotoxin_model_execution_parameter;

  explicit constexpr Parameter(Api api) : api(api) {}

  auto get_field() const -> Ttx::Concept::Abstract {
    return Ttx::Concept::Abstract(api.get_field(api.source));
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Model::Execution

TTX_DATA_RECORD(
    tetrodotoxin_model_execution_parameter,
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_parameter, source),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_parameter, get_field));
