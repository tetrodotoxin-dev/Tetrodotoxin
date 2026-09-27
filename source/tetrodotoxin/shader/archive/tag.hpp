// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Shader::Archive {

enum class Tag : U16 {
  Monograph = 1,
  Program,
  Binding,
  Uniform,
  Bridge,
};

}  // namespace Tetrodotoxin::Shader::Archive
