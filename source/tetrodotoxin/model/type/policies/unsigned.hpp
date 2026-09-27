// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "tetrodotoxin/model/type/policies/unsigned.h"

namespace Tetrodotoxin::Model::Type::Policies {

// Unsigned promises that the numeric interpretation excludes negative values.
// Consumers can ask for this property without acquiring an API or a storage
// description. Width, arithmetic and overflow rules belong to other contracts,
// so a substitute can preserve this promise while choosing another carrier.
class Unsigned {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_TYPE_UNSIGNED_ID_HIGH,
    TETRODOTOXIN_MODEL_TYPE_UNSIGNED_ID_LOW,
  };
  using Api = void;
};

}  // namespace Tetrodotoxin::Model::Type::Policies
