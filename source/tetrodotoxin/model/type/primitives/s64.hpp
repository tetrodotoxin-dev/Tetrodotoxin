// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/primitives/scalar.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// The S64 provider composes its numeric policy with the native S64 carrier.
using S64 = Scalar<::S64>;

}  // namespace Tetrodotoxin::Model::Type::Primitives
