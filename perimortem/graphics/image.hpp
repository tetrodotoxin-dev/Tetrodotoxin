// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/pixel.hpp"
#include "perimortem/graphics/size_2d.hpp"

namespace Perimortem::Graphics {

// Image is one shared decoded RGBA identity. Copies retain the same worker
// local content, while target images and sampling policy remain independent
// runtime facts.
class Image {
 public:
  Image();
  Image(U32 width, U32 height);
  Image(Memory::Dynamic::Vector<Pixel>&& source, U32 width, U32 height);
  Image(const Image& source);
  Image(Image&& source);
  ~Image();

  auto operator=(const Image& source) -> Image&;
  auto operator=(Image&& source) -> Image&;

  auto get_width() const -> U32;
  auto get_height() const -> U32;
  auto get_size_pixels() const -> Size2D;
  // The returned row-major pixels borrow this Image lifetime.
  auto get_pixels() const -> Core::View::Vector<Pixel>;
  auto get_pixel(S32 x, S32 y) const -> Pixel;
  auto is_drawable() const -> Bool;
  constexpr auto get_object() const -> Core::Object<> { return object; }

  static auto retain(Core::Object<> object) -> Core::Option<Image>;

  static constexpr auto get_color_depth() -> U8 { return color_depth; }
  static constexpr auto get_channel_count() -> U8 { return channel_count; }

 private:
  class Payload {
   public:
    Core::Object<Pixel> pixels;
    Count pixel_count = 0;
    Size2D size_pixels;
  };
  static_assert(__builtin_offsetof(Payload, pixels) == 0);
  static_assert(__builtin_offsetof(Payload, pixel_count) == sizeof(U8*));
  static_assert(
      __builtin_offsetof(Payload, size_pixels) == sizeof(U8*) + sizeof(Count));
  static_assert(
      sizeof(Payload) == sizeof(U8*) + sizeof(Count) + sizeof(Size2D));

  explicit Image(Core::Object<> object) : object(object) {}
  static auto finalize(U8* payload) -> void;
  auto get_payload() -> Payload&;
  auto get_payload() const -> const Payload&;

  static constexpr U8 color_depth = 8;
  static constexpr U8 channel_count = 4;
  static const Core::Object<>::Descriptor descriptor;
  Core::Object<> object;
};

static_assert(sizeof(Image) == sizeof(U8*));
static_assert(alignof(Image) == alignof(U8*));

}  // namespace Perimortem::Graphics
