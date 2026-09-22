// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/sampler_2d.hpp"

namespace Perimortem::Graphics {

// Texture2D is one inline sampled view of shared Image content. It adds no
// allocation identity, so several values can select one Image with independent
// sampling policy.
class Texture2D {
 public:
  Texture2D() = default;
  explicit Texture2D(const Image& image, Sampler2D sampler = {})
      : image(image), sampler(sampler) {}

  constexpr auto get_image() const -> const Image& { return image; }
  constexpr auto get_sampler() const -> Sampler2D { return sampler; }
  auto get_size_pixels() const -> Size2D { return image.get_size_pixels(); }
  auto is_drawable() const -> Bool { return image.is_drawable(); }

 private:
  Image image;
  Sampler2D sampler;
};

static_assert(sizeof(Texture2D) == sizeof(U8*) * 2);
static_assert(alignof(Texture2D) == alignof(U8*));
static_assert(__is_standard_layout(Texture2D));

}  // namespace Perimortem::Graphics
