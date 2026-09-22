// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/primitives/scalar.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// The S8 provider composes its numeric policy with the native S8 carrier.
using S8 = Scalar<::S8>;

}  // namespace Tetrodotoxin::Model::Type::Primitives
