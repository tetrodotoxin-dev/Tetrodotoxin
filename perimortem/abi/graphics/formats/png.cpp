// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/formats/png.hpp"

#include "perimortem/core/view/bytes.hpp"

using namespace Perimortem;

struct PngBytes {
  const U8* data;
  U64 size;
};

static_assert(sizeof(PngBytes) == sizeof(U8*) + sizeof(U64));
static_assert(alignof(PngBytes) == alignof(U64));
static_assert(__is_trivial(PngBytes));
static_assert(__is_standard_layout(PngBytes));

// Option over one nonnull authored Object uses the invalid null handle as its
// absent state. A successful result transfers one Image reservation directly
// into the generated caller.
extern "C" auto perimortem_graphics_png_decode(PngBytes source) -> U8* {
  Graphics::Image image = Graphics::Formats::Png::decode(
      Core::View::Bytes(source.data, source.size));
  if (!image.is_drawable()) {
    return nullptr;
  }

  Core::Object<> object = image.get_object();
  object.retain();
  return object.get_payload();
}
