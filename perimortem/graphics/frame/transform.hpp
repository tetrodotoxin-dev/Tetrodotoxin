// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics::Frame {

// Transform is the backend neutral affine value copied into a completed frame.
// Hosted values can therefore change their source state after submission
// without changing the transform observed by the renderer.
class Transform {
 public:
  constexpr Transform() = default;

  static auto create(
      R64 translation_x,
      R64 translation_y,
      R64 scale_x,
      R64 scale_y,
      R64 rotation) -> Transform;

  static auto compose(const Transform& parent, const Transform& local)
      -> Transform;

  constexpr auto get_xx() const -> R64 { return xx; }
  constexpr auto get_xy() const -> R64 { return xy; }
  constexpr auto get_yx() const -> R64 { return yx; }
  constexpr auto get_yy() const -> R64 { return yy; }
  constexpr auto get_x() const -> R64 { return x; }
  constexpr auto get_y() const -> R64 { return y; }

  constexpr auto operator==(const Transform& rhs) const -> Bool {
    return xx == rhs.xx && xy == rhs.xy && yx == rhs.yx && yy == rhs.yy &&
           x == rhs.x && y == rhs.y;
  }

 private:
  constexpr Transform(R64 xx, R64 xy, R64 yx, R64 yy, R64 x, R64 y)
      : xx(xx), xy(xy), yx(yx), yy(yy), x(x), y(y) {}

  R64 xx = 1.0;
  R64 xy = 0.0;
  R64 yx = 0.0;
  R64 yy = 1.0;
  R64 x = 0.0;
  R64 y = 0.0;
};

}  // namespace Perimortem::Graphics::Frame
