// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics {

// The canonical decoded RGBA pixel with eight bits per channel. Public channel
// fields are intentional data oriented storage. Image algorithms can process a
// continuous Pixel buffer without accessors obscuring the four byte layout.
// Encoded formats with another channel order or depth are converted at the
// codec boundary rather than changing the meaning of this runtime value.
class Pixel {
 public:
  // Fully transparent black is the zero state.
  Pixel() = default;

  // Replicates grey to all color channels and uses full opacity.
  static constexpr auto from_grey(U8 grey) -> Pixel {
    return from_rgba(grey, grey, grey, opaque);
  }

  // Replicates grey to all color channels with an explicit alpha value.
  static constexpr auto from_grey_alpha(U8 grey, U8 alpha) -> Pixel {
    return from_rgba(grey, grey, grey, alpha);
  }

  // Stores three independent color channels and uses full opacity.
  static constexpr auto from_rgb(U8 red, U8 green, U8 blue) -> Pixel {
    return from_rgba(red, green, blue, opaque);
  }

  // Stores all four channels directly.
  static constexpr auto from_rgba(U8 red, U8 green, U8 blue, U8 alpha)
      -> Pixel {
    Pixel result;
    result.red = red;
    result.green = green;
    result.blue = blue;
    result.alpha = alpha;
    return result;
  }

  static constexpr auto get_bit_depth() -> Count { return 8; }
  static constexpr auto get_byte_count() -> Count { return 4; }

  U8 red = 0;
  U8 green = 0;
  U8 blue = 0;
  U8 alpha = 0;

 private:
  static constexpr U8 opaque = 0xFF;
};

static_assert(sizeof(Pixel) == 4);

}  // namespace Perimortem::Graphics
