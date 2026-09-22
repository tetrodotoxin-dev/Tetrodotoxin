// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Terminal::Spirv {

// Target selects the validation environment whose physical rules the module
// follows. Shader meaning remains target neutral until this request boundary.
enum class Target : U8 {
  Vulkan1_0,
};

}  // namespace Tetrodotoxin::Terminal::Spirv
