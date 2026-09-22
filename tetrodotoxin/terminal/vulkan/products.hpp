// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Tetrodotoxin::Terminal::Vulkan {

// Products is the source independent Vulkan description derived from one
// completed Shader Program and its exact Pipeline contract. Every record is a
// physical target fact and retains no semantic identity after generation.
class Products {
 public:
  enum class Stage : U8 {
    Vertex,
    Pixel,
  };

  enum class HostRole : U8 {
    Parameter,
    TransformX,
    TransformY,
  };

  enum class Resource : U8 {
    SampledTexture2D,
  };

  struct Entry {
    Stage stage;
    Perimortem::Core::View::Bytes name;
  };

  struct Descriptor {
    Perimortem::Core::View::Bytes name;
    Count set;
    Count slot;
    Resource resource;
  };

  struct VertexInput {
    Count location;
    Count components;
    Count offset;
    Count stride;
  };

  struct HostField {
    Perimortem::Core::View::Bytes name;
    Count offset;
    Count size;
    HostRole role;
  };

  constexpr Products(
      Perimortem::Core::View::Bytes symbol,
      Perimortem::Core::View::Vector<Entry> entries,
      Perimortem::Core::View::Vector<Descriptor> descriptors,
      Perimortem::Core::View::Vector<VertexInput> vertex_inputs,
      Perimortem::Core::View::Vector<HostField> host_fields,
      Count host_size,
      Count parameters_offset,
      Count parameters_size,
      Bool requires_float64)
      : symbol(symbol),
        entries(entries),
        descriptors(descriptors),
        vertex_inputs(vertex_inputs),
        host_fields(host_fields),
        host_size(host_size),
        parameters_offset(parameters_offset),
        parameters_size(parameters_size),
        requires_float64(requires_float64) {}

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }
  constexpr auto get_entries() const -> Perimortem::Core::View::Vector<Entry> {
    return entries;
  }
  constexpr auto get_descriptors() const
      -> Perimortem::Core::View::Vector<Descriptor> {
    return descriptors;
  }
  constexpr auto get_vertex_inputs() const
      -> Perimortem::Core::View::Vector<VertexInput> {
    return vertex_inputs;
  }
  constexpr auto get_host_fields() const
      -> Perimortem::Core::View::Vector<HostField> {
    return host_fields;
  }
  constexpr auto get_host_size() const -> Count { return host_size; }
  constexpr auto get_parameters_offset() const -> Count {
    return parameters_offset;
  }
  constexpr auto get_parameters_size() const -> Count {
    return parameters_size;
  }
  constexpr auto needs_float64() const -> Bool { return requires_float64; }

 private:
  Perimortem::Core::View::Bytes symbol;
  Perimortem::Core::View::Vector<Entry> entries;
  Perimortem::Core::View::Vector<Descriptor> descriptors;
  Perimortem::Core::View::Vector<VertexInput> vertex_inputs;
  Perimortem::Core::View::Vector<HostField> host_fields;
  Count host_size;
  Count parameters_offset;
  Count parameters_size;
  Bool requires_float64;
};

}  // namespace Tetrodotoxin::Terminal::Vulkan
