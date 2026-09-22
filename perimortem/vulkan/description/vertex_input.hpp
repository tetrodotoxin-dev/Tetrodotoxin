// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Vulkan::Description {

// VertexInput describes one interleaved R32 vector consumed at a Shader
// location. Offset and stride belong to the selected Vulkan product rather
// than the target-neutral Pipeline contract.
struct VertexInput {
  Count location = 0;
  Count components = 0;
  Count offset = 0;
  Count stride = 0;
};

}  // namespace Perimortem::Vulkan::Description
