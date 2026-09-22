// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/frame/batch.hpp"

using namespace Perimortem;

Graphics::Frame::Batch::Batch(
    Program program,
    Memory::Dynamic::Vector<Resource>&& resources,
    Memory::Dynamic::Bytes&& inputs,
    Transform transform,
    Size2D size_pixels,
    Pipeline pipeline,
    Count vertex_count,
    S64 z_index,
    Count authored_order)
    : program(program),
      resources(static_cast<Memory::Dynamic::Vector<Resource>&&>(resources)),
      inputs(static_cast<Memory::Dynamic::Bytes&&>(inputs)),
      transform(transform),
      size_pixels(size_pixels),
      pipeline(pipeline),
      vertex_count(vertex_count),
      z_index(z_index),
      authored_order(authored_order) {}

auto Graphics::Frame::Batch::get_inputs() const -> Core::View::Bytes {
  return inputs.get_view();
}
