// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/graphics/frame/transform.hpp"
#include "perimortem/graphics/point_2d.hpp"

namespace Perimortem::Graphics {

// Transform2D is the authored placement value shared by every hosted Object.
// Frame collection converts it once so mutable source state cannot alter a
// submission already owned by the renderer.
struct Transform2D {
  auto to_frame() const -> Frame::Transform {
    return Frame::Transform::create(
        translation.x, translation.y, scale_x, scale_y, rotation);
  }

  Point2D translation;
  R64 scale_x = 1.0;
  R64 scale_y = 1.0;
  R64 rotation = 0.0;
};

static_assert(sizeof(Transform2D) == sizeof(R64) * 5);
static_assert(__is_standard_layout(Transform2D));

}  // namespace Perimortem::Graphics
