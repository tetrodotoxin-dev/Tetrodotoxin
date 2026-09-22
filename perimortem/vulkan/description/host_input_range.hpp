// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/vulkan/description/stage.hpp"

namespace Perimortem::Vulkan::Description {

// Describes one byte range of host data made visible to the listed Vulkan
// stages. The range is pipeline metadata only. Draw bytes arrive later through
// the stable Graphics submission and are never retained here.
struct HostInputRange {
  Count offset = 0;
  Count size = 0;
  Core::View::Vector<Stage> stages;
};

}  // namespace Perimortem::Vulkan::Description
