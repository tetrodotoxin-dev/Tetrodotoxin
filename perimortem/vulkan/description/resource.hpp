// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Vulkan::Description {

// Resource identifies the generated Vulkan resource realization selected by a
// generated Pipeline resource.
enum class Resource : U8 {
  SampledTexture2D,
};

}  // namespace Perimortem::Vulkan::Description
