// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/vulkan/description/program.hpp"
#include "tetrodotoxin/graphics/runtime/children_2d.hpp"
#include "tetrodotoxin/graphics/runtime/drawable_2d.hpp"
#include "tetrodotoxin/graphics/runtime/placement_2d.hpp"
#include "tetrodotoxin/runtime/application/scene.hpp"
#include "tetrodotoxin/runtime/application/transition.hpp"

namespace Tetrodotoxin::Runtime::Application {

using PlacementProvider =
    const Tetrodotoxin::Graphics::Runtime::Placement2D* (*)();
using ChildrenProvider =
    const Tetrodotoxin::Graphics::Runtime::Children2D* (*)();
using DrawableProvider =
    const Tetrodotoxin::Graphics::Runtime::Drawable2D* (*)();

// Product is the immutable handoff from the App Terminal to the runtime. The
// arrays and title bytes live in the generated entry object for the complete
// process lifetime.
struct Product {
  const U8* title;
  U32 width;
  U32 height;
  const Scene* scenes;
  Count scene_count;
  Count initial_scene;
  const Transition* transitions;
  Count transition_count;
  const PlacementProvider* graphics_placements;
  const ChildrenProvider* graphics_children;
  const DrawableProvider* graphics_drawables;
  Count graphics_type_count;
  const Perimortem::Vulkan::Description::Program* programs;
  Count program_count;
};

static_assert(__is_trivial(Product));

}  // namespace Tetrodotoxin::Runtime::Application
