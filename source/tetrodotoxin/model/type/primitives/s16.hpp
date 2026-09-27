// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/primitives/scalar.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// The S16 provider composes its numeric policy with the native S16 carrier.
using S16 = Scalar<::S16>;

}  // namespace Tetrodotoxin::Model::Type::Primitives
