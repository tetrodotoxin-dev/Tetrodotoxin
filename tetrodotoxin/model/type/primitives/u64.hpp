// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/primitives/scalar.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// The U64 provider composes its numeric policy with the native U64 carrier.
using U64 = Scalar<::U64>;

}  // namespace Tetrodotoxin::Model::Type::Primitives
