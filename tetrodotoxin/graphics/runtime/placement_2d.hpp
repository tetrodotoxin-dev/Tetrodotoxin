// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/graphics/frame/transform.hpp"

namespace Tetrodotoxin::Graphics::Runtime {

// Placement2D exposes only the frame placement of one native Object. Scene
// traversal can consume this capability without acquiring child or drawing
// behavior from the same provider.
class Placement2D {
 public:
  class Placement {
   public:
    constexpr Placement() = default;
    constexpr Placement(
        Perimortem::Graphics::Frame::Transform transform,
        Bool visible,
        S64 z_index)
        : transform(transform), visible(visible), z_index(z_index) {}

    constexpr auto get_transform() const
        -> const Perimortem::Graphics::Frame::Transform& {
      return transform;
    }
    constexpr auto is_visible() const -> Bool { return visible; }
    constexpr auto get_z_index() const -> S64 { return z_index; }

   private:
    Perimortem::Graphics::Frame::Transform transform;
    Bool visible = True;
    S64 z_index = 0;
  };

  using Read = Placement (*)(const U8*, Perimortem::Core::Object<>);

  constexpr Placement2D(const U8* context, Read read)
      : context(context), read(read) {}

  auto placement(Perimortem::Core::Object<> object) const
      -> Perimortem::Core::Option<Placement>;

 private:
  const U8* context;
  Read read;
};

}  // namespace Tetrodotoxin::Graphics::Runtime
