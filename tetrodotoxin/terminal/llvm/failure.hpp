// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Terminal::Llvm {

// SourceRejected means the Terminal published an actionable source report.
// ToolchainFailed means the process diagnostic log owns the failure detail.
enum class Failure : U8 {
  SourceRejected,
  ToolchainFailed,
};

}  // namespace Tetrodotoxin::Terminal::Llvm
