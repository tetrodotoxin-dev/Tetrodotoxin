// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/runtime/compiled_children_2d.hpp"

#include "perimortem/core/data.hpp"

using namespace Tetrodotoxin::Graphics;

Runtime::CompiledChildren2D::CompiledChildren2D(
    Count child_count,
    Runtime::CompiledChildren2D::ReadChild read_child)
    : retained_child_count(child_count),
      retained_read_child(read_child),
      children(
          Perimortem::Core::Data::cast<const U8>(this),
          read_child_count,
          read_selected_child) {}

auto Runtime::CompiledChildren2D::read_child_count(
    const U8* context,
    Perimortem::Core::Object<> object) -> Count {
  const auto* selected =
      Perimortem::Core::Data::cast<const Runtime::CompiledChildren2D>(context);
  return object.is_empty() || selected->retained_read_child == nullptr
             ? 0
             : selected->retained_child_count;
}

auto Runtime::CompiledChildren2D::read_selected_child(
    const U8* context,
    Perimortem::Core::Object<> object,
    Count index,
    Perimortem::Core::Object<>* child) -> Count {
  const auto* selected =
      Perimortem::Core::Data::cast<const Runtime::CompiledChildren2D>(context);
  if (object.is_empty() || selected->retained_read_child == nullptr ||
      child == nullptr || index >= selected->retained_child_count) {
    return Count(-1);
  }

  void* payload = nullptr;
  Count type_index =
      selected->retained_read_child(object.get_payload(), index, &payload);
  if (payload == nullptr || type_index == Count(-1)) {
    return Count(-1);
  }

  *child =
      Perimortem::Core::Object<>(Perimortem::Core::Data::cast<U8>(payload));
  return type_index;
}
