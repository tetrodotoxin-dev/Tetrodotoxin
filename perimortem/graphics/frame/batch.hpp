// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/object.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/frame/pipeline.hpp"
#include "perimortem/graphics/frame/program.hpp"
#include "perimortem/graphics/frame/resource.hpp"
#include "perimortem/graphics/frame/transform.hpp"
#include "perimortem/graphics/size_2d.hpp"

namespace Perimortem::Graphics::Frame {

// Batch is one immutable backend neutral draw transaction. Program locators
// have process lifetime, while resource and input values are retained or copied
// for this exact frame.
class Batch {
 public:
  Batch() = default;

  Batch(
      Program program,
      Memory::Dynamic::Vector<Resource>&& resources,
      Memory::Dynamic::Bytes&& inputs,
      Transform transform,
      Size2D size_pixels,
      Pipeline pipeline,
      Count vertex_count,
      S64 z_index,
      Count authored_order);

  constexpr auto get_program() const -> Program { return program; }
  constexpr auto get_resources() const -> Core::View::Vector<Resource> {
    return resources.get_view();
  }
  auto get_inputs() const -> Core::View::Bytes;
  constexpr auto get_transform() const -> const Transform& { return transform; }
  constexpr auto get_size_pixels() const -> Size2D { return size_pixels; }
  constexpr auto get_pipeline() const -> Pipeline { return pipeline; }
  constexpr auto get_vertex_count() const -> Count { return vertex_count; }
  constexpr auto get_z_index() const -> S64 { return z_index; }
  constexpr auto get_authored_order() const -> Count { return authored_order; }
  constexpr auto operator>(const Batch& rhs) const -> Bool {
    return z_index > rhs.z_index ||
           (z_index == rhs.z_index && authored_order > rhs.authored_order);
  }

 private:
  Program program;
  Memory::Dynamic::Vector<Resource> resources;
  Memory::Dynamic::Bytes inputs;
  Transform transform;
  Size2D size_pixels;
  Pipeline pipeline;
  Count vertex_count = 0;
  S64 z_index = 0;
  Count authored_order = 0;
};

}  // namespace Perimortem::Graphics::Frame
