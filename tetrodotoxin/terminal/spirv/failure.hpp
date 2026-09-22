// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Terminal::Spirv {

enum class Failure : U8 {
  SourceRejected,
  ToolchainFailed,
};

}  // namespace Tetrodotoxin::Terminal::Spirv
