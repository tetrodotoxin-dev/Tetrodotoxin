// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/primitives/scalar.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// The R64 provider composes its numeric policy with the native R64 carrier.
using R64 = Scalar<::R64>;

}  // namespace Tetrodotoxin::Model::Type::Primitives
