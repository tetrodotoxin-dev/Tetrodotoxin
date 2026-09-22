// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/vulkan/description/host_role.hpp"

namespace Perimortem::Vulkan::Description {

// Identifies one named field in the host input layout. The offset and size are
// retained as reflection data so Vulkan can populate the selected layout
// without retaining compiler Types.
struct HostField {
  Core::View::Bytes name;
  Count offset = 0;
  Count size = 0;
  HostRole role = HostRole::Parameter;
};

}  // namespace Perimortem::Vulkan::Description
