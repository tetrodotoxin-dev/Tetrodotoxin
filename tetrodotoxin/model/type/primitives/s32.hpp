// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/primitives/scalar.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// The S32 provider composes its numeric policy with the native S32 carrier.
using S32 = Scalar<::S32>;

}  // namespace Tetrodotoxin::Model::Type::Primitives
