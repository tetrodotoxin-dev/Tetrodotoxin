// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "tetrodotoxin/model/type/policies/real.h"

namespace Tetrodotoxin::Model::Type::Policies {

// Real promises an IEEE floating point interpretation. The selected storage
// representation establishes its concrete encoding, while this property lets
// a semantic consumer ask about that interpretation without acquiring bytes.
class Real {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_TYPE_REAL_ID_HIGH,
    TETRODOTOXIN_MODEL_TYPE_REAL_ID_LOW,
  };
  using Api = void;
};

}  // namespace Tetrodotoxin::Model::Type::Policies
