// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/constant.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Execution {

// A terminal needs an immutable payload before it can embed an observation in
// generated code. This property supplies that promise while Domain and Value
// supply the Type and data interface. It does not make conversion decisions,
// write permissions or Abstract get_data observations constant.
class Constant {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_EXECUTION_CONSTANT_ID_HIGH,
    TETRODOTOXIN_MODEL_EXECUTION_CONSTANT_ID_LOW,
  };
  using Api = void;
};

}  // namespace Tetrodotoxin::Model::Execution
