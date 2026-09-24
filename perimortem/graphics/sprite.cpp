// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/sprite.hpp"

using namespace Perimortem;

auto Graphics::Sprite::get_texture() const -> const Texture2D& {
  return texture;
}

auto Graphics::Sprite::set_texture(const Texture2D& texture) -> void {
  this->texture = texture;
}

auto Graphics::Sprite::get_material() const -> const Core::Implementation& {
  return material;
}

auto Graphics::Sprite::set_material(const Core::Implementation& material)
    -> void {
  this->material = material;
}

auto Graphics::Sprite::get_size() const -> Size2D {
  return size;
}

auto Graphics::Sprite::set_size(Size2D size) -> void {
  this->size = size;
}

auto Graphics::Sprite::get_transform() const -> Transform2D {
  return transform;
}

auto Graphics::Sprite::set_transform(Transform2D transform) -> void {
  this->transform = transform;
}

auto Graphics::Sprite::is_visible() const -> Bool {
  return visible;
}

auto Graphics::Sprite::set_visible(Bool visible) -> void {
  this->visible = visible;
}

auto Graphics::Sprite::get_z_index() const -> S64 {
  return z_index;
}

auto Graphics::Sprite::set_z_index(S64 z_index) -> void {
  this->z_index = z_index;
}

auto Graphics::Sprite::is_drawable() const -> Bool {
  return visible && size.width != 0 && size.height != 0 &&
         texture.is_drawable() && material.is_valid();
}
