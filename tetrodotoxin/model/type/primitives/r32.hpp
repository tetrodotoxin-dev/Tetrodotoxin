// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/primitives/scalar.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// The R32 provider composes its numeric policy with the native R32 carrier.
using R32 = Scalar<::R32>;

}  // namespace Tetrodotoxin::Model::Type::Primitives
