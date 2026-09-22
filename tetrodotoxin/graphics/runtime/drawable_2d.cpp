// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/runtime/drawable_2d.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Graphics;

auto Runtime::Drawable2D::draw_count(Object<> object) const -> Count {
  return !object.is_empty() && read_draw_count != nullptr
             ? read_draw_count(object)
             : 0;
}

auto Runtime::Drawable2D::draw(Object<> object, Count index) const
    -> Option<Draw> {
  BAIL_IF(
      object.is_empty() || read_draw == nullptr || index >= draw_count(object));
  Draw selected = read_draw(object, index);
  BAIL_IF(!selected.get_program().is_valid());
  return selected;
}
