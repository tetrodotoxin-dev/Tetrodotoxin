// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Terminal::Spirv::Module {

// Ids allocates the one monotonically increasing result namespace required by
// a SPIR V module. The final bound remains one greater than every issued id.
class Ids {
 public:
  constexpr auto take() -> U32 {
    U32 selected = next;
    next++;
    return selected;
  }

  constexpr auto get_bound() const -> U32 { return next; }

 private:
  U32 next = 1;
};

}  // namespace Tetrodotoxin::Terminal::Spirv::Module
