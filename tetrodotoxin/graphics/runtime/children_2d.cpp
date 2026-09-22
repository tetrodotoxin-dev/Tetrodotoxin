// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/runtime/children_2d.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Graphics;

auto Runtime::Children2D::child_count(Object<> object) const -> Count {
  return !object.is_empty() && read_count != nullptr
             ? read_count(context, object)
             : 0;
}

auto Runtime::Children2D::child(Object<> object, Count index) const
    -> Option<Child> {
  BAIL_IF(object.is_empty() || read == nullptr || index >= child_count(object));
  Object<> selected;
  Count type_index = read(context, object, index, &selected);
  BAIL_IF(selected.is_empty() || type_index == Count(-1));
  return Child(selected, type_index);
}
