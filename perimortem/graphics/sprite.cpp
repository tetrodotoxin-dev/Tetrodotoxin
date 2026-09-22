// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/sprite.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem;

extern "C" const Core::Object<>::Descriptor
    TTX_DESC_Perimortem_2eGraphics__Sprite__Sprite __attribute__((weak));

const Core::Object<>::Descriptor Graphics::Sprite::descriptor(
    sizeof(Payload),
    alignof(Payload),
    Graphics::Sprite::finalize);

Graphics::Sprite::Sprite() : object(Core::Object<>::create(descriptor)) {
  new (object.get_payload(), Core::Placement::Construct) Payload();
}

Graphics::Sprite::Sprite(const Sprite& source) : object(source.object) {
  object.retain();
}

Graphics::Sprite::Sprite(Sprite&& source) : object(source.object) {
  source.object = Core::Object<>();
}

Graphics::Sprite::~Sprite() {
  object.release();
}

auto Graphics::Sprite::operator=(const Sprite& source) -> Sprite& {
  if (object.get_payload() == source.object.get_payload()) {
    return *this;
  }

  source.object.retain();
  object.release();
  object = source.object;
  return *this;
}

auto Graphics::Sprite::operator=(Sprite&& source) -> Sprite& {
  if (this == &source) {
    return *this;
  }

  object.release();
  object = source.object;
  source.object = Core::Object<>();
  return *this;
}

auto Graphics::Sprite::get_texture() const -> const Texture2D& {
  return get_payload().texture;
}

auto Graphics::Sprite::set_texture(const Texture2D& texture) -> void {
  get_payload().texture = texture;
}

auto Graphics::Sprite::get_material() const -> const Core::Implementation& {
  return get_payload().material;
}

auto Graphics::Sprite::set_material(const Core::Implementation& material)
    -> void {
  get_payload().material = material;
}

auto Graphics::Sprite::get_size() const -> Size2D {
  return get_payload().size;
}

auto Graphics::Sprite::set_size(Size2D size) -> void {
  get_payload().size = size;
}

auto Graphics::Sprite::get_transform() const -> Transform2D {
  return get_payload().transform;
}

auto Graphics::Sprite::set_transform(Transform2D transform) -> void {
  get_payload().transform = transform;
}

auto Graphics::Sprite::is_visible() const -> Bool {
  return get_payload().visible;
}

auto Graphics::Sprite::set_visible(Bool visible) -> void {
  get_payload().visible = visible;
}

auto Graphics::Sprite::get_z_index() const -> S64 {
  return get_payload().z_index;
}

auto Graphics::Sprite::set_z_index(S64 z_index) -> void {
  get_payload().z_index = z_index;
}

auto Graphics::Sprite::is_drawable() const -> Bool {
  const Payload& payload = get_payload();
  return payload.visible && payload.size.width != 0 &&
         payload.size.height != 0 && payload.texture.is_drawable() &&
         payload.material.is_valid();
}

auto Graphics::Sprite::retain(Core::Object<> object) -> Core::Option<Sprite> {
  BAIL_IF(object.is_empty());
  const Core::Object<>::Descriptor* generated =
      &TTX_DESC_Perimortem_2eGraphics__Sprite__Sprite;
  const Core::Object<>::Descriptor& selected = object.get_descriptor();
  BAIL_IF(
      &selected != &Sprite::descriptor &&
      (generated == nullptr || &selected != generated));
  object.retain();
  return Sprite(object);
}

auto Graphics::Sprite::finalize(U8* payload) -> void {
  Core::Data::cast<Payload>(payload)->~Payload();
}

auto Graphics::Sprite::get_payload() -> Payload& {
  return *Core::Data::cast<Payload>(object.get_payload());
}

auto Graphics::Sprite::get_payload() const -> const Payload& {
  return *Core::Data::cast<const Payload>(object.get_payload());
}
