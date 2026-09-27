// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/primitives/scalar.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// The U8 provider composes its numeric policy with the native U8 carrier.
using U8 = Scalar<::U8>;

}  // namespace Tetrodotoxin::Model::Type::Primitives
