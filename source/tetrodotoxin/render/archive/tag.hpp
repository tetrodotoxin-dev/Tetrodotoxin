// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Render::Archive {

enum class Tag : U16 {
  Monograph = 1,
  Alias,
  Binding,
  Stage,
  Structure,
  Layout,
  Slot,
};

}  // namespace Tetrodotoxin::Render::Archive
