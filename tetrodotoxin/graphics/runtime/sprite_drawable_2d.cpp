// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/runtime/sprite_drawable_2d.hpp"

#include "perimortem/core/data.hpp"

#include "perimortem/graphics/projection.hpp"
#include "perimortem/graphics/sprite.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Graphics;

auto Runtime::SpriteDrawable2D::get_runtime() -> const Drawable2D& {
  static const Drawable2D drawable(read_draw_count, read_draw);
  return drawable;
}

auto Runtime::SpriteDrawable2D::read_draw_count(Object<> object) -> Count {
  auto sprite = Perimortem::Graphics::Sprite::retain(object);
  return sprite && sprite->is_drawable() ? 1 : 0;
}

auto Runtime::SpriteDrawable2D::read_draw(Object<> object, Count index)
    -> Drawable2D::Draw {
  auto sprite = Perimortem::Graphics::Sprite::retain(object);
  if (!sprite || !sprite->is_drawable() || index != 0) {
    return {};
  }

  const Implementation& implementation = sprite->get_material();
  const auto* projection = Data::cast<const Perimortem::Graphics::Projection>(
      implementation.get_projection());
  Object<> instance = implementation.get_object();
  if (!implementation.is_valid() || projection == nullptr ||
      projection->program == nullptr || instance.is_empty()) {
    return {};
  }
  Count instance_size = instance.get_descriptor().get_size();
  if (projection->parameters_offset > instance_size ||
      projection->parameters_size >
          instance_size - projection->parameters_offset) {
    return {};
  }
  Perimortem::Memory::Dynamic::Bytes inputs(
      View::Bytes(
          instance.get_payload() + projection->parameters_offset,
          projection->parameters_size));
  Perimortem::Memory::Dynamic::Vector<Perimortem::Graphics::Frame::Resource>
      resources;
  if (projection->resources == nullptr || projection->resource_count == 0) {
    return {};
  }
  for (Count resource_index = 0; resource_index < projection->resource_count;
       resource_index++) {
    const Perimortem::Graphics::Projection::Resource& projected =
        projection->resources[resource_index];
    const Perimortem::Graphics::Texture2D* texture = nullptr;
    switch (projected.source) {
    case Perimortem::Graphics::Projection::ResourceSource::HostTexture:
      texture = &sprite->get_texture();
      break;
    case Perimortem::Graphics::Projection::ResourceSource::InstanceTexture:
      if (projected.offset > instance_size ||
          sizeof(Perimortem::Graphics::Texture2D) >
              instance_size - projected.offset) {
        return {};
      }
      texture = Data::cast<const Perimortem::Graphics::Texture2D>(
          instance.get_payload() + projected.offset);
      break;
    }
    if (texture == nullptr || !texture->is_drawable()) {
      return {};
    }
    auto retained =
        Perimortem::Graphics::Frame::Resource::retain_texture(*texture);
    if (retained.is_empty()) {
      return {};
    }
    resources.emplace(
        static_cast<Perimortem::Graphics::Frame::Resource&&>(retained));
  }
  return Drawable2D::Draw(
      Perimortem::Graphics::Frame::Program(projection->program),
      static_cast<Perimortem::Memory::Dynamic::Vector<
          Perimortem::Graphics::Frame::Resource>&&>(resources),
      static_cast<Perimortem::Memory::Dynamic::Bytes&&>(inputs),
      sprite->get_size(),
      Perimortem::Graphics::Frame::Pipeline(
          Perimortem::Graphics::Frame::Pipeline::Topology::TriangleList,
          Perimortem::Graphics::Frame::Pipeline::Blend::Alpha,
          Perimortem::Graphics::Frame::Pipeline::Geometry::UnitQuad2D),
      6, 0);
}

extern "C" auto tetrodotoxin_graphics_sprite_drawable_2d()
    -> const Runtime::Drawable2D* {
  return &Runtime::SpriteDrawable2D::get_runtime();
}
