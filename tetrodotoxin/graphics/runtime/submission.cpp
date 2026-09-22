// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/runtime/submission.hpp"

#include "perimortem/core/algorithm/sort.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Graphics;

auto Runtime::PassUI::collect(
    Collection& collection,
    Object<> object,
    const Placement2D* placement,
    const Children2D* children,
    const Drawable2D* drawable,
    const Perimortem::Graphics::Frame::Transform& parent,
    S64 parent_z_index) -> Bool {
  BAIL_IF(object.is_empty());
  Perimortem::Graphics::Frame::Resource reservation(object);

  Perimortem::Graphics::Frame::Transform transform = parent;
  S64 z_index = parent_z_index;
  if (placement != nullptr) {
    auto selected = placement->placement(object);
    BAIL_IF(!selected);
    if (!selected->is_visible()) {
      return True;
    }
    transform = Perimortem::Graphics::Frame::Transform::compose(
        parent, selected->get_transform());
    BAIL_IF(__builtin_add_overflow(
        parent_z_index, selected->get_z_index(), &z_index));
  }

  BAIL_IF(collection.path.contains(object.get_payload()));

  collection.path.insert(object.get_payload());
  Bool complete =
      collect_draws(collection, object, drawable, transform, z_index) &&
      collect_children(collection, object, children, transform, z_index);
  collection.path.remove(collection.path.get_size() - 1);
  return complete;
}

auto Runtime::PassUI::collect_draws(
    Collection& collection,
    Object<> object,
    const Drawable2D* drawable,
    const Perimortem::Graphics::Frame::Transform& transform,
    S64 placement_z_index) -> Bool {
  if (drawable == nullptr) {
    return True;
  }

  Count count = drawable->draw_count(object);
  for (Count index = 0; index < count; index++) {
    auto draw = drawable->draw(object, index);
    BAIL_IF(!draw);
    S64 z_index = 0;
    BAIL_IF(__builtin_add_overflow(
        placement_z_index, draw->get_z_offset(), &z_index));
    collection.batches.emplace(
        Perimortem::Graphics::Frame::Batch(
            draw->get_program(), draw->take_resources(), draw->take_inputs(),
            transform, draw->get_size_pixels(), draw->get_pipeline(),
            draw->get_vertex_count(), z_index, collection.order++));
  }
  return True;
}

auto Runtime::PassUI::collect_children(
    Collection& collection,
    Object<> object,
    const Children2D* children,
    const Perimortem::Graphics::Frame::Transform& transform,
    S64 z_index) -> Bool {
  if (children == nullptr) {
    return True;
  }
  Count count = children->child_count(object);
  for (Count index = 0; index < count; index++) {
    auto child = children->child(object, index);
    BAIL_IF(
        !child || child->get_type_index() >= collection.placements.get_size());
    Count type_index = child->get_type_index();
    const Placement2D* placement = collection.placements.get_data()[type_index];
    const Children2D* selected_children =
        collection.children.get_data()[type_index];
    const Drawable2D* drawable = collection.drawables.get_data()[type_index];
    BAIL_IF(
        placement == nullptr && selected_children == nullptr &&
        drawable == nullptr);
    BAIL_IF(!collect(
        collection, child->get_object(), placement, selected_children, drawable,
        transform, z_index));
  }
  return True;
}

auto Runtime::PassUI::create(
    Object<> root,
    const Children2D& root_children,
    View::Vector<const Placement2D*> placements,
    View::Vector<const Children2D*> children,
    View::Vector<const Drawable2D*> drawables) -> Option<PassUI> {
  BAIL_IF(
      root.is_empty() || placements.is_empty() ||
      placements.get_size() != children.get_size() ||
      placements.get_size() != drawables.get_size());
  Collection collection(placements, children, drawables);
  BAIL_IF(!collect(
      collection, root, nullptr, &root_children, nullptr,
      Perimortem::Graphics::Frame::Transform(), 0));
  Algorithm::sort(collection.batches.get_access());
  return PassUI(
      static_cast<Dynamic::Vector<Perimortem::Graphics::Frame::Batch>&&>(
          collection.batches));
}
