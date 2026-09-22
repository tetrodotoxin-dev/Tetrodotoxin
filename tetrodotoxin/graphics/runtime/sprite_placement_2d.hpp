// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/graphics/runtime/placement_2d.hpp"

namespace Tetrodotoxin::Graphics::Runtime {

// SpritePlacement2D exposes the native Sprite placement independently from
// its drawable projection.
class SpritePlacement2D {
 public:
  static auto get_runtime() -> const Placement2D&;

 private:
  static auto read(const U8* context, Perimortem::Core::Object<> object)
      -> Placement2D::Placement;
};

}  // namespace Tetrodotoxin::Graphics::Runtime

extern "C" auto tetrodotoxin_graphics_sprite_placement_2d()
    -> const Tetrodotoxin::Graphics::Runtime::Placement2D*;
