// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/graphics/runtime/children_2d.hpp"

namespace Tetrodotoxin::Graphics::Runtime {

// CompiledChildren2D exposes one generated Scene child accessor through the
// independent Children2D capability. The generated function already owns
// authored Field order and native offsets.
class CompiledChildren2D {
 public:
  using ReadChild = Count (*)(void*, Count, void**);

  CompiledChildren2D(Count child_count, ReadChild read_child);

  CompiledChildren2D(const CompiledChildren2D&) = delete;
  CompiledChildren2D(CompiledChildren2D&&) = delete;
  auto operator=(const CompiledChildren2D&) -> CompiledChildren2D& = delete;
  auto operator=(CompiledChildren2D&&) -> CompiledChildren2D& = delete;

  constexpr auto get_children() const -> const Children2D& { return children; }

 private:
  static auto read_child_count(
      const U8* context,
      Perimortem::Core::Object<> object) -> Count;
  static auto read_selected_child(
      const U8* context,
      Perimortem::Core::Object<> object,
      Count index,
      Perimortem::Core::Object<>* child) -> Count;

  Count retained_child_count;
  ReadChild retained_read_child;
  Children2D children;
};

}  // namespace Tetrodotoxin::Graphics::Runtime
