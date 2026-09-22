// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/image.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"

using namespace Perimortem;

extern "C" const Core::Object<>::Descriptor
    TTX_DESC_Perimortem_2eGraphics__Image__Image __attribute__((weak));

const Core::Object<>::Descriptor Graphics::Image::descriptor(
    sizeof(Payload),
    alignof(Payload),
    Graphics::Image::finalize);

static auto create_pixels(
    const Memory::Dynamic::Vector<Graphics::Pixel>& source,
    Count count) -> Core::Object<Graphics::Pixel> {
  Core::Object<Graphics::Pixel> pixels(count);
  auto target = pixels.get_access();
  Count retained = Core::Math::min(source.get_size(), count);
  for (Count index = 0; index < retained; index++) {
    target.get_data()[index] = source[index];
  }

  return pixels;
}

Graphics::Image::Image() : object(Core::Object<>::create(descriptor)) {
  new (object.get_payload(), Core::Placement::Construct) Payload();
}

Graphics::Image::Image(U32 width, U32 height) : Image() {
  Payload& payload = get_payload();
  payload.pixels = Core::Object<Pixel>(Count(width) * Count(height));
  payload.pixel_count = Count(width) * Count(height);
  payload.size_pixels = {width, height};
}

Graphics::Image::Image(
    Memory::Dynamic::Vector<Pixel>&& source,
    U32 width,
    U32 height)
    : Image() {
  Payload& payload = get_payload();
  payload.pixels = create_pixels(source, Count(width) * Count(height));
  payload.pixel_count = Count(width) * Count(height);
  payload.size_pixels = {width, height};
}

Graphics::Image::Image(const Image& source) : object(source.object) {
  object.retain();
}

Graphics::Image::Image(Image&& source) : object(source.object) {
  source.object = Core::Object<>();
}

Graphics::Image::~Image() {
  object.release();
}

auto Graphics::Image::operator=(const Image& source) -> Image& {
  if (object.get_payload() == source.object.get_payload()) {
    return *this;
  }

  source.object.retain();
  object.release();
  object = source.object;
  return *this;
}

auto Graphics::Image::operator=(Image&& source) -> Image& {
  if (this == &source) {
    return *this;
  }

  object.release();
  object = source.object;
  source.object = Core::Object<>();
  return *this;
}

auto Graphics::Image::get_width() const -> U32 {
  return get_payload().size_pixels.width;
}

auto Graphics::Image::get_height() const -> U32 {
  return get_payload().size_pixels.height;
}

auto Graphics::Image::get_size_pixels() const -> Size2D {
  return get_payload().size_pixels;
}

auto Graphics::Image::get_pixels() const -> Core::View::Vector<Pixel> {
  const Payload& payload = get_payload();
  return payload.pixels.get_view().slice(0, payload.pixel_count);
}

auto Graphics::Image::get_pixel(S32 x, S32 y) const -> Pixel {
  const Payload& payload = get_payload();
  if (x < 0 || x >= payload.size_pixels.width || y < 0 ||
      y >= payload.size_pixels.height) {
    return Pixel();
  }

  return payload.pixels.get_view()
      .get_data()[Count(y) * Count(payload.size_pixels.width) + Count(x)];
}

auto Graphics::Image::is_drawable() const -> Bool {
  const Payload& payload = get_payload();
  return payload.size_pixels.width != 0 && payload.size_pixels.height != 0 &&
         payload.pixel_count ==
             Count(payload.size_pixels.width) * payload.size_pixels.height &&
         payload.pixels.get_capacity() >= payload.pixel_count;
}

auto Graphics::Image::retain(Core::Object<> object)
    -> Core::Option<Image> {
  BAIL_IF(object.is_empty());
  const Core::Object<>::Descriptor* generated =
      &TTX_DESC_Perimortem_2eGraphics__Image__Image;
  const Core::Object<>::Descriptor& selected = object.get_descriptor();
  BAIL_IF(
      &selected != &descriptor &&
      (generated == nullptr || &selected != generated));
  object.retain();
  return Image(object);
}

auto Graphics::Image::finalize(U8* payload) -> void {
  Core::Data::cast<Payload>(payload)->~Payload();
}

auto Graphics::Image::get_payload() -> Payload& {
  return *Core::Data::cast<Payload>(object.get_payload());
}

auto Graphics::Image::get_payload() const -> const Payload& {
  return *Core::Data::cast<const Payload>(object.get_payload());
}
