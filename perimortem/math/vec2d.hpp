// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Math {

// Vec2D is the native value carrier for the standard Math::Vec2D Type.
// Keeping the same two public components lets generated ABI structures retain
// the authored Type instead of flattening it into unrelated scalar fields.
struct Vec2D {
  R32 x;
  R32 y;
};

static_assert(sizeof(Vec2D) == sizeof(R32) * 2);
static_assert(alignof(Vec2D) == alignof(R32));
static_assert(__is_trivial(Vec2D));
static_assert(__is_standard_layout(Vec2D));

}  // namespace Perimortem::Math
