// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/frame/pipeline.hpp"
#include "perimortem/graphics/frame/program.hpp"
#include "perimortem/graphics/frame/resource.hpp"
#include "perimortem/graphics/size_2d.hpp"

namespace Tetrodotoxin::Graphics::Runtime {

// Drawable2D is the runtime Interface for Objects that contribute draw meaning.
// Exact Program selection belongs to each Draw, which lets placement and
// rendering evolve independently.
class Drawable2D {
 public:
  // Draw is one immutable contribution copied out of a live Object. Resources
  // retain their real runtime identities while inputs freeze the values the
  // selected Program needs for this frame.
  class Draw {
   public:
    Draw() = default;
    Draw(
        Perimortem::Graphics::Frame::Program program,
        Perimortem::Memory::Dynamic::Vector<
            Perimortem::Graphics::Frame::Resource>&& resources,
        Perimortem::Memory::Dynamic::Bytes&& inputs,
        Perimortem::Graphics::Size2D size_pixels,
        Perimortem::Graphics::Frame::Pipeline pipeline,
        Count vertex_count,
        S64 z_offset)
        : program(program),
          resources(
              static_cast<Perimortem::Memory::Dynamic::Vector<
                  Perimortem::Graphics::Frame::Resource>&&>(resources)),
          inputs(static_cast<Perimortem::Memory::Dynamic::Bytes&&>(inputs)),
          size_pixels(size_pixels),
          pipeline(pipeline),
          vertex_count(vertex_count),
          z_offset(z_offset) {}

    constexpr auto get_program() const -> Perimortem::Graphics::Frame::Program {
      return program;
    }
    constexpr auto get_resources() const -> Perimortem::Core::View::Vector<
        Perimortem::Graphics::Frame::Resource> {
      return resources.get_view();
    }
    auto get_inputs() const -> Perimortem::Core::View::Bytes {
      return inputs.get_view();
    }
    constexpr auto get_size_pixels() const -> Perimortem::Graphics::Size2D {
      return size_pixels;
    }
    constexpr auto get_pipeline() const
        -> Perimortem::Graphics::Frame::Pipeline {
      return pipeline;
    }
    constexpr auto get_vertex_count() const -> Count { return vertex_count; }
    constexpr auto get_z_offset() const -> S64 { return z_offset; }
    auto take_resources() -> Perimortem::Memory::Dynamic::Vector<
        Perimortem::Graphics::Frame::Resource>&& {
      return static_cast<Perimortem::Memory::Dynamic::Vector<
          Perimortem::Graphics::Frame::Resource>&&>(resources);
    }
    auto take_inputs() -> Perimortem::Memory::Dynamic::Bytes&& {
      return static_cast<Perimortem::Memory::Dynamic::Bytes&&>(inputs);
    }

   private:
    Perimortem::Graphics::Frame::Program program;
    Perimortem::Memory::Dynamic::Vector<Perimortem::Graphics::Frame::Resource>
        resources;
    Perimortem::Memory::Dynamic::Bytes inputs;
    Perimortem::Graphics::Size2D size_pixels;
    Perimortem::Graphics::Frame::Pipeline pipeline;
    Count vertex_count = 0;
    S64 z_offset = 0;
  };

  using ReadDrawCount = Count (*)(Perimortem::Core::Object<>);
  using ReadDraw = Draw (*)(Perimortem::Core::Object<>, Count);

  constexpr Drawable2D(ReadDrawCount read_draw_count, ReadDraw read_draw)
      : read_draw_count(read_draw_count), read_draw(read_draw) {}

  auto draw_count(Perimortem::Core::Object<> object) const -> Count;
  auto draw(Perimortem::Core::Object<> object, Count index) const
      -> Perimortem::Core::Option<Draw>;

 private:
  ReadDrawCount read_draw_count;
  ReadDraw read_draw;
};

}  // namespace Tetrodotoxin::Graphics::Runtime
