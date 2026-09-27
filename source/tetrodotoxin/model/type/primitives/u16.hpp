// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/primitives/scalar.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// The U16 provider composes its numeric policy with the native U16 carrier.
using U16 = Scalar<::U16>;

}  // namespace Tetrodotoxin::Model::Type::Primitives
