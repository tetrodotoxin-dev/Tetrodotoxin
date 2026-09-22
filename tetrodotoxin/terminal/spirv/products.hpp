// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Terminal::Spirv {

// Products owns the completed module view in the caller Arena. The bytes carry
// target representation only and retain no live Shader or Library identity.
class Products {
 public:
  constexpr explicit Products(Perimortem::Core::View::Bytes module)
      : module(module) {}

  constexpr auto get_module() const -> Perimortem::Core::View::Bytes {
    return module;
  }

 private:
  Perimortem::Core::View::Bytes module;
};

}  // namespace Tetrodotoxin::Terminal::Spirv
