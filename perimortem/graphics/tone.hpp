// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics {

// Tone modulates a rendered color in linear component order. Opaque white is
// the semantic default because an unmodified image should preserve its source.
struct Tone {
  R64 red = 1.0;
  R64 green = 1.0;
  R64 blue = 1.0;
  R64 alpha = 1.0;
};

static_assert(sizeof(Tone) == sizeof(R64) * 4);
static_assert(__is_standard_layout(Tone));

}  // namespace Perimortem::Graphics
