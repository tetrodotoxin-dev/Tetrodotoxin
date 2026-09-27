// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

#include "tetrodotoxin/graphics/runtime/compiled_children_2d.hpp"

namespace Tetrodotoxin::Runtime::Application {

// Scene describes the native entry points produced from one completed Scene
// graph. The live Object remains opaque here, while each callback keeps the C
// ABI selected by the Terminal product.
struct Scene {
  using Construct = void* (*)();
  using Lifecycle = void (*)(void**);
  using Update = void (*)(void**, R64);

  Construct construct;
  Lifecycle prepare;
  Lifecycle pause;
  Lifecycle resume;
  Update update;
  Lifecycle release;
  Count graphics_child_count;
  Tetrodotoxin::Graphics::Runtime::CompiledChildren2D::ReadChild
      graphics_children;
};

static_assert(__is_trivial(Scene));

}  // namespace Tetrodotoxin::Runtime::Application
