// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/type.hpp"

namespace Tetrodotoxin::Terminal::Spirv {

// Layout derives the logical SPIR V shape shared by module decoration and a
// consuming Vulkan pipeline description. The result is physical target data,
// while the queried Library Type remains the sole semantic owner.
class Layout {
 public:
  class Measurement {
   public:
    constexpr Measurement(Count size, Count alignment)
        : size(size), alignment(alignment) {}

    constexpr auto get_size() const -> Count { return size; }
    constexpr auto get_alignment() const -> Count { return alignment; }

   private:
    Count size;
    Count alignment;
  };

  Layout() = delete;

  static auto get_vector_components(
      const Tetrodotoxin::Library::Language::Model::Type& type)
      -> Perimortem::Core::Option<Count>;

  static auto measure(const Tetrodotoxin::Library::Language::Model::Type& type)
      -> Perimortem::Core::Option<Measurement>;
};

}  // namespace Tetrodotoxin::Terminal::Spirv
