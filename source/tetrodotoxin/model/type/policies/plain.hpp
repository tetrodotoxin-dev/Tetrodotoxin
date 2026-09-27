// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "tetrodotoxin/model/type/policies/plain.h"

namespace Tetrodotoxin::Model::Type::Policies {

// Plain promises that an admitted value can be reproduced from its complete
// storage representation without constructors, resource acquisition or hidden
// value constraints. The receiving conversion policy can therefore admit an
// identical representation without inspecting every field as an Abstract.
// Additional restrictions belong to a policy that governs that admission.
class Plain {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_TYPE_PLAIN_ID_HIGH,
    TETRODOTOXIN_MODEL_TYPE_PLAIN_ID_LOW,
  };
  using Api = void;
};

}  // namespace Tetrodotoxin::Model::Type::Policies
