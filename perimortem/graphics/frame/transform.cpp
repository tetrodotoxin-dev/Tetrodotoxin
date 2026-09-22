// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/frame/transform.hpp"

using namespace Perimortem;

auto Graphics::Frame::Transform::create(
    R64 translation_x,
    R64 translation_y,
    R64 scale_x,
    R64 scale_y,
    R64 rotation) -> Transform {
  R64 cosine = __builtin_cos(rotation);
  R64 sine = __builtin_sin(rotation);
  return Transform(
      cosine * scale_x, -sine * scale_y, sine * scale_x, cosine * scale_y,
      translation_x, translation_y);
}

auto Graphics::Frame::Transform::compose(
    const Transform& parent,
    const Transform& local) -> Transform {
  return Transform(
      parent.xx * local.xx + parent.xy * local.yx,
      parent.xx * local.xy + parent.xy * local.yy,
      parent.yx * local.xx + parent.yy * local.yx,
      parent.yx * local.xy + parent.yy * local.yy,
      parent.xx * local.x + parent.xy * local.y + parent.x,
      parent.yx * local.x + parent.yy * local.y + parent.y);
}
