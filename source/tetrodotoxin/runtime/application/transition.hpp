// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/runtime/application/action.hpp"

namespace Tetrodotoxin::Runtime::Application {

// Transition uses the stable Signal token emitted beside its defining Scene.
// Scene indices select the product descriptors without copying semantic names
// into the runtime.
struct Transition {
  Count source;
  const U8* signal;
  Action action;
  Count destination;
};

static_assert(__is_trivial(Transition));

}  // namespace Tetrodotoxin::Runtime::Application
