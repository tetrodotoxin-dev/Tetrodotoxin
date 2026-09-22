// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/runtime/sprite_placement_2d.hpp"

#include "perimortem/graphics/sprite.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Graphics;

auto Runtime::SpritePlacement2D::get_runtime() -> const Placement2D& {
  static const Placement2D placement(nullptr, read);
  return placement;
}

auto Runtime::SpritePlacement2D::read(const U8*, Object<> object)
    -> Placement2D::Placement {
  auto sprite = Perimortem::Graphics::Sprite::retain(object);
  if (!sprite) {
    return {};
  }
  return Placement2D::Placement(
      sprite->get_transform().to_frame(), sprite->is_visible(),
      sprite->get_z_index());
}

extern "C" auto tetrodotoxin_graphics_sprite_placement_2d()
    -> const Runtime::Placement2D* {
  return &Runtime::SpritePlacement2D::get_runtime();
}
