// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/frame/batch.hpp"
#include "perimortem/graphics/frame/transform.hpp"
#include "tetrodotoxin/graphics/runtime/children_2d.hpp"
#include "tetrodotoxin/graphics/runtime/drawable_2d.hpp"
#include "tetrodotoxin/graphics/runtime/placement_2d.hpp"

namespace Tetrodotoxin::Graphics::Runtime {

// PassUI collects DrawableUI Objects into ascending z order for one frame. The
// configured runtime providers stay independent and share only the Type index
// emitted by the Scene compiler.
class PassUI {
 public:
  static auto create(
      Perimortem::Core::Object<> root,
      const Children2D& root_children,
      Perimortem::Core::View::Vector<const Placement2D*> placements,
      Perimortem::Core::View::Vector<const Children2D*> children,
      Perimortem::Core::View::Vector<const Drawable2D*> drawables)
      -> Perimortem::Core::Option<PassUI>;

  constexpr auto get_batches() const
      -> Perimortem::Core::View::Vector<Perimortem::Graphics::Frame::Batch> {
    return batches.get_view();
  }

 private:
  class Collection {
   public:
    constexpr Collection(
        Perimortem::Core::View::Vector<const Placement2D*> placements,
        Perimortem::Core::View::Vector<const Children2D*> children,
        Perimortem::Core::View::Vector<const Drawable2D*> drawables)
        : placements(placements), children(children), drawables(drawables) {}

    Perimortem::Core::View::Vector<const Placement2D*> placements;
    Perimortem::Core::View::Vector<const Children2D*> children;
    Perimortem::Core::View::Vector<const Drawable2D*> drawables;
    Perimortem::Memory::Dynamic::Vector<Perimortem::Graphics::Frame::Batch>
        batches;
    Perimortem::Memory::Dynamic::Vector<const U8*> path;
    Count order = 0;
  };

  static auto collect(
      Collection& collection,
      Perimortem::Core::Object<> object,
      const Placement2D* placement,
      const Children2D* children,
      const Drawable2D* drawable,
      const Perimortem::Graphics::Frame::Transform& parent,
      S64 parent_z_index) -> Bool;
  static auto collect_draws(
      Collection& collection,
      Perimortem::Core::Object<> object,
      const Drawable2D* drawable,
      const Perimortem::Graphics::Frame::Transform& transform,
      S64 placement_z_index) -> Bool;
  static auto collect_children(
      Collection& collection,
      Perimortem::Core::Object<> object,
      const Children2D* children,
      const Perimortem::Graphics::Frame::Transform& transform,
      S64 z_index) -> Bool;

  explicit PassUI(
      Perimortem::Memory::Dynamic::Vector<Perimortem::Graphics::Frame::Batch>&&
          batches)
      : batches(
            static_cast<Perimortem::Memory::Dynamic::Vector<
                Perimortem::Graphics::Frame::Batch>&&>(batches)) {}

  Perimortem::Memory::Dynamic::Vector<Perimortem::Graphics::Frame::Batch>
      batches;
};

}  // namespace Tetrodotoxin::Graphics::Runtime
