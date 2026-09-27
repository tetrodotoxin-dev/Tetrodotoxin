// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "tetrodotoxin/model/type/policies/signed.h"

namespace Tetrodotoxin::Model::Type::Policies {

// Signed promises a numeric interpretation that includes negative values.
// It supplies no storage width or arithmetic operations. Keeping this question
// independent lets consumers ask about sign without selecting a native type.
class Signed {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_TYPE_SIGNED_ID_HIGH,
    TETRODOTOXIN_MODEL_TYPE_SIGNED_ID_LOW,
  };
  using Api = void;
};

}  // namespace Tetrodotoxin::Model::Type::Policies
