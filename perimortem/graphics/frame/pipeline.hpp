// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics::Frame {

// Pipeline is the backend-neutral fixed state selected by one draw provider.
// A Program supplies executable stages and bindings, while the Object that
// contributes geometry supplies the state needed to realize those stages.
class Pipeline {
 public:
  enum class Topology : U8 {
    TriangleList,
  };

  enum class Blend : U8 {
    Alpha,
  };

  enum class Geometry : U8 {
    UnitQuad2D,
  };

  constexpr Pipeline() = default;

  constexpr Pipeline(Topology topology, Blend blend, Geometry geometry)
      : topology(topology), blend(blend), geometry(geometry) {}

  constexpr auto get_topology() const -> Topology { return topology; }
  constexpr auto get_blend() const -> Blend { return blend; }
  constexpr auto get_geometry() const -> Geometry { return geometry; }

  constexpr auto operator==(const Pipeline& rhs) const -> Bool {
    return topology == rhs.topology && blend == rhs.blend &&
           geometry == rhs.geometry;
  }

 private:
  Topology topology = Topology::TriangleList;
  Blend blend = Blend::Alpha;
  Geometry geometry = Geometry::UnitQuad2D;
};

}  // namespace Perimortem::Graphics::Frame
