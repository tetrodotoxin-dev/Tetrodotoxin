// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/runtime/application/product.hpp"

namespace Tetrodotoxin::Runtime::Application {

// Runner realizes one completed Windowed App product. It owns the frame loop
// and coordinates System input, Scene lifecycle, Graphics submission, and
// Vulkan presentation without retaining another semantic model.
class Runner {
 public:
  Runner() = delete;

  static auto run(const Product& product) -> int;
};

}  // namespace Tetrodotoxin::Runtime::Application

extern "C" auto tetrodotoxin_application_scene(
    const Tetrodotoxin::Runtime::Application::Product* product) -> int;
