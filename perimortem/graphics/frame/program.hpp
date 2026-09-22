// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics::Frame {

// Program is the process lifetime locator for one compiled rendering contract.
// A frame preserves that relationship without owning a backend's program
// representation. The caller keeps the matching compiled product available to
// the selected renderer.
class Program {
 public:
  constexpr Program() = default;
  explicit constexpr Program(const U8* locator) : locator(locator) {}

  constexpr auto is_valid() const -> Bool { return locator != nullptr; }
  constexpr auto get_locator() const -> const U8* { return locator; }
  constexpr auto operator==(const Program& rhs) const -> Bool {
    return locator == rhs.locator;
  }

 private:
  const U8* locator = nullptr;
};

}  // namespace Perimortem::Graphics::Frame
