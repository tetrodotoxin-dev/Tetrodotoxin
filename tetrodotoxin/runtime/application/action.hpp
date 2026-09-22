// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Runtime::Application {

// Action is the runtime policy selected by one completed App transition. It is
// deliberately smaller than the App graph because the Terminal has already
// resolved routes and checked which actions require a destination.
enum class Action : U8 {
  Replace,
  Push,
  Pop,
  Exit,
};

}  // namespace Tetrodotoxin::Runtime::Application
