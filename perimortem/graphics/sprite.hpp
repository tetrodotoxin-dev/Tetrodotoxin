// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/implementation.hpp"

#include "perimortem/graphics/size_2d.hpp"
#include "perimortem/graphics/texture_2d.hpp"
#include "perimortem/graphics/transform_2d.hpp"

namespace Perimortem::Graphics {

// Sprite stores the values needed to place a textured image. Copying one gives
// the caller independent placement and visibility, while its Texture2D and
// material retain the resources they refer to. Sharing a Sprite itself belongs
// to the owner that publishes it.
class Sprite {
 public:
  Sprite() = default;

  auto get_texture() const -> const Texture2D&;
  auto set_texture(const Texture2D& texture) -> void;
  auto get_material() const -> const Core::Implementation&;
  auto set_material(const Core::Implementation& material) -> void;
  auto get_size() const -> Size2D;
  auto set_size(Size2D size) -> void;
  auto get_transform() const -> Transform2D;
  auto set_transform(Transform2D transform) -> void;
  auto is_visible() const -> Bool;
  auto set_visible(Bool visible) -> void;
  auto get_z_index() const -> S64;
  auto set_z_index(S64 z_index) -> void;
  auto is_drawable() const -> Bool;

 private:
  Core::Implementation material;
  Transform2D transform;
  Bool visible = True;
  S64 z_index = 0;
  Texture2D texture;
  Size2D size;
};

}  // namespace Perimortem::Graphics
