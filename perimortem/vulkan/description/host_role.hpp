// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Vulkan::Description {

// HostRole tells Vulkan which generated host value supplies one push field.
enum class HostRole : U8 {
  Parameter,
  TransformX,
  TransformY,
};

}  // namespace Perimortem::Vulkan::Description
