// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/graphics/image.hpp"

namespace Perimortem::Graphics::Formats {

// Stateless PNG codec that converts between encoded bytes and RGBA Images.
class Png {
 public:
  // Decodes a PNG byte stream into a Graphics::Image (RGBA pixels, row major).
  // All source color formats are expanded to full RGBA.
  //
  // Returns an empty Image on malformed or unsupported input.
  //
  // CRC 32 validation is skipped in release builds to improve throughput.
  //
  // Benchmarked on AMD Ryzen 9 9950X3D, Arch Linux 6.19.11, using libpng 1.6.58
  // as a reference (CRC disabled for parity):
  //
  //   Image         Perimortem   libpng decode
  //   gradient        134 MB/s         63 MB/s
  //   complex         100 MB/s        100 MB/s
  //   noise           288 MB/s        265 MB/s
  //   icon             51 MB/s         51 MB/s
  //   mandelbrot       43 MB/s        	30 MB/s
  //   zoneplate       143 MB/s        105 MB/s
  //
  static auto decode(Core::View::Bytes source) -> Image;

  // Encodes an Image to a PNG byte stream using an adaptive filtering heuristic
  // which scores each line using all five filter types, selecting the one with
  // the lowest heuristic score.
  //
  // Returns empty bytes for any bad inputs and empty images.
  //
  // For compression Level::Default is used for all PNGs as it produces the most
  // well rounded results.
  //
  // Benchmarked on AMD Ryzen 9 9950X3D, Arch Linux 6.19.11, using libpng 1.6.58
  // as a reference using level 6 compression:
  //
  //   Image         Perimortem   libpng encode     Compression Ratios
  //   gradient         48 MB/s          7 MB/s       0.99 (smaller)
  //   complex          29 MB/s          5 MB/s       1.05 (bigger)
  //   noise            43 MB/s         22 MB/s       1.02 (bigger)
  //   icon             55 MB/s          9 MB/s       1.01 (bigger)
  //   mandelbrot       16 MB/s          3 MB/s       1.04 (much bigger)
  //   zoneplate        33 MB/s          7 MB/s       1.40 (much bigger)
  //
  static auto encode(const Image& image) -> Memory::Dynamic::Bytes;
};

}  // namespace Perimortem::Graphics::Formats
