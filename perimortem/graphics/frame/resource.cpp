// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/frame/resource.hpp"

using namespace Perimortem::Core;
using namespace Perimortem;

Graphics::Frame::Resource::Resource(Object<> object) : object(object) {
  object.retain();
}

auto Graphics::Frame::Resource::retain_texture(
    const Graphics::Texture2D& texture) -> Resource {
  BAIL_IF(!texture.is_drawable());
  Resource resource(texture.get_image().get_object());
  resource.sampler = texture.get_sampler();
  return resource;
}

Graphics::Frame::Resource::Resource(const Resource& source)
    : object(source.object), sampler(source.sampler) {
  object.retain();
}

Graphics::Frame::Resource::Resource(Resource&& source)
    : object(source.object), sampler(source.sampler) {
  source.object = Object<>();
}

Graphics::Frame::Resource::~Resource() {
  object.release();
}

auto Graphics::Frame::Resource::operator=(const Resource& source) -> Resource& {
  if (object.get_payload() == source.object.get_payload()) {
    sampler = source.sampler;
    return *this;
  }

  source.object.retain();
  object.release();
  object = source.object;
  sampler = source.sampler;
  return *this;
}

auto Graphics::Frame::Resource::operator=(Resource&& source) -> Resource& {
  if (this == &source) {
    return *this;
  }

  object.release();
  object = source.object;
  sampler = source.sampler;
  source.object = Object<>();
  return *this;
}

auto Graphics::Frame::Resource::get_reservations() const -> Count {
  return object.get_reservations();
}
