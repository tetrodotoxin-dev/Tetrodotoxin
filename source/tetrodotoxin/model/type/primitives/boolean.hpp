// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/primitives/scalar.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// Boolean uses a byte carrier and supplies truth observation through Flag.
// Sharing that carrier with U8 does not make their semantic policies identical.
using Boolean = Scalar<bool>;

}  // namespace Tetrodotoxin::Model::Type::Primitives
