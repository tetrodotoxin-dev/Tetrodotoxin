// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics {

// Sampler2D is target-neutral sampling policy carried independently from one
// shared Image. Its compact value can be copied into a frame without acquiring
// another allocation identity.
class Sampler2D {
 public:
  enum class Addressing : U8 {
    Zero,
    Clamp,
    Wrap,
  };

  enum class Filtering : U8 {
    Linear,
    Nearest,
  };

  constexpr Sampler2D() = default;
  constexpr Sampler2D(Addressing addressing, Filtering filtering)
      : addressing(addressing), filtering(filtering) {}

  constexpr auto get_addressing() const -> Addressing { return addressing; }
  constexpr auto get_filtering() const -> Filtering { return filtering; }

  constexpr auto operator==(Sampler2D other) const -> Bool {
    return addressing == other.addressing && filtering == other.filtering;
  }

 private:
  Addressing addressing = Addressing::Zero;
  Filtering filtering = Filtering::Linear;
};

static_assert(sizeof(Sampler2D) == sizeof(U8) * 2);
static_assert(alignof(Sampler2D) == alignof(U8));
static_assert(__is_trivially_copyable(Sampler2D));
static_assert(__is_standard_layout(Sampler2D));

}  // namespace Perimortem::Graphics
