// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/pixel.hpp"
#include "perimortem/graphics/size_2d.hpp"

namespace Perimortem::Graphics {

// Image owns decoded RGBA pixels and their dimensions. Creation fits a pixel
// buffer to the requested rectangle and reports invalid dimensions through
// Option. Pixel format and content validation belong to the supplying codec.
// Copies own independent pixel storage. Move assignment exchanges complete
// values so the donor owns the displaced buffer.
//
// Pixel observations borrow the Image. Consumers that need to share its
// lifetime can place it in a Record, as Texture2D does, without changing
// Image's layout or making every Image allocation carry that policy.
class Image {
 public:
  constexpr Image() = default;
  constexpr Image(const Image&) = default;
  constexpr Image(Image&& source) {
    Core::Data::swap(pixels, source.pixels);
    Core::Data::swap(size_pixels, source.size_pixels);
  }

  constexpr auto operator=(const Image&) -> Image& = default;
  constexpr auto operator=(Image&& source) -> Image& {
    if (this != &source) {
      Core::Data::swap(pixels, source.pixels);
      Core::Data::swap(size_pixels, source.size_pixels);
    }

    return *this;
  }

  // The caller chooses whether to copy or transfer the buffer. Missing pixels
  // are filled with zero bytes, including alpha. An oversized buffer keeps its
  // allocation and capacity, with only the requested rectangle exposed.
  // Zero dimensions or an unrepresentable byte extent return None before
  // fitting the buffer. Allocation failure follows Vector's allocator policy.
  static auto create(Size2D size, Memory::Dynamic::Vector<Pixel> source)
      -> Core::Option<Image> {
    const Count count = Count(size.width) * size.height;
    if (!count || count > CppSize(-1) / sizeof(Pixel)) {
      return Core::Option<Image>();
    }

    source.resize(count);
    Image image;
    Core::Data::swap(image.pixels, source);
    image.size_pixels = size;
    return image;
  }

  constexpr auto get_width() const -> U32 { return size_pixels.width; }
  constexpr auto get_height() const -> U32 { return size_pixels.height; }
  constexpr auto get_size_pixels() const -> Size2D { return size_pixels; }
  constexpr auto get_pixels() const -> Core::View::Vector<Pixel> {
    return pixels.get_view();
  }

  constexpr auto get_pixel(S32 x, S32 y) const -> Pixel {
    if (x < 0 || x >= size_pixels.width || y < 0 || y >= size_pixels.height) {
      return Pixel();
    }

    return pixels[Count(y) * size_pixels.width + Count(x)];
  }

  constexpr auto is_drawable() const -> Bool {
    return size_pixels.width && size_pixels.height;
  }

  static constexpr auto get_color_depth() -> U8 { return 8; }
  static constexpr auto get_channel_count() -> U8 { return 4; }

 private:
  Memory::Dynamic::Vector<Pixel> pixels;
  Size2D size_pixels;
};

}  // namespace Perimortem::Graphics
