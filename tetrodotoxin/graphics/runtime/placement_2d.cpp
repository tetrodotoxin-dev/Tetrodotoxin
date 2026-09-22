// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/runtime/placement_2d.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Graphics;

auto Runtime::Placement2D::placement(Object<> object) const
    -> Option<Placement> {
  BAIL_IF(object.is_empty() || read == nullptr);
  return read(context, object);
}
