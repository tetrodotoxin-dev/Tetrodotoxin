// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics {

// Point2D keeps graphics position distinct from a general Math vector. The
// representation is deliberately plain because the native reference and the
// generated TTX surface share these two values directly.
struct Point2D {
  S32 x = 0.0;
  S32 y = 0.0;
};

static_assert(sizeof(Point2D) == sizeof(S32) * 2);
static_assert(__is_standard_layout(Point2D));

}  // namespace Perimortem::Graphics
