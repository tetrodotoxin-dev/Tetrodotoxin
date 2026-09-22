// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/vulkan/description/resource.hpp"

namespace Perimortem::Vulkan::Description {

// Gives a reflected resource its stable name and Vulkan binding location. The
// name remains available while Vulkan constructs its independently owned
// descriptor state.
struct DescriptorBinding {
  Core::View::Bytes name;
  Count set = 0;
  Count slot = 0;
  Resource resource = Resource::SampledTexture2D;
};

}  // namespace Perimortem::Vulkan::Description
