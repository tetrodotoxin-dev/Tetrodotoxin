// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Vulkan::Description {

// Identifies the Vulkan pipeline stage that consumes a module or host input
// range. Conversion to native flags remains local to pipeline construction.
enum class Stage : U32 {
  Vertex,
  Pixel,
};

}  // namespace Perimortem::Vulkan::Description
