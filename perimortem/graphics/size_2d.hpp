// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics {

// Size2D describes a pixel extent without borrowing storage or choosing a
// backend image. A zero extent is the natural unconfigured value.
struct Size2D {
  U32 width = 0;
  U32 height = 0;
};

static_assert(sizeof(Size2D) == sizeof(U32) * 2);
static_assert(__is_standard_layout(Size2D));

}  // namespace Perimortem::Graphics
