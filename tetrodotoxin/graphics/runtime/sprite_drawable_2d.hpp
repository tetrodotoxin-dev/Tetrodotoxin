// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/graphics/runtime/drawable_2d.hpp"

namespace Tetrodotoxin::Graphics::Runtime {

// SpriteDrawable2D projects the native Sprite into one TexturedQuad2D draw.
// The selected Program is bound at application composition, while texture and
// material values continue to come from the live Sprite Object.
class SpriteDrawable2D {
 public:
  static auto get_runtime() -> const Drawable2D&;

 private:
  static auto read_draw_count(Perimortem::Core::Object<> object) -> Count;
  static auto read_draw(Perimortem::Core::Object<> object, Count index)
      -> Drawable2D::Draw;
};

}  // namespace Tetrodotoxin::Graphics::Runtime

extern "C" auto tetrodotoxin_graphics_sprite_drawable_2d()
    -> const Tetrodotoxin::Graphics::Runtime::Drawable2D*;
