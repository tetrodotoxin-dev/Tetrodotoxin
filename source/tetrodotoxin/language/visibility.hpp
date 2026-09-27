// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Language {

// Visibility is the authored access mode retained by one Definition. Concrete
// languages decide which modes are legal for the semantic object they create.
enum class Visibility : ::U8 {
  Private,
  Public,
  Exposed,
};

}  // namespace Tetrodotoxin::Language
