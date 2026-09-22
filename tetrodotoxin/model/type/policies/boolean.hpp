// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "tetrodotoxin/model/type/policies/boolean.h"

namespace Tetrodotoxin::Model::Type::Policies {

// A truth operation alone cannot prove that copying its input bytes produces
// a Boolean. This property identifies the false and true value domain, while
// Flag supplies an operation that may narrow a much richer domain to truth.
class Boolean {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_TYPE_BOOLEAN_ID_HIGH,
    TETRODOTOXIN_MODEL_TYPE_BOOLEAN_ID_LOW,
  };
  using Api = void;
};

}  // namespace Tetrodotoxin::Model::Type::Policies
