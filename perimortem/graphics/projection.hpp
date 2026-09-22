// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics {

// Projection maps one concrete Shader Instance Object to target ABI facts. The
// record has process lifetime and contains no semantic graph pointer. Program
// selects the compiled Shader product while the byte range selects the exact
// Parameters subobject owned by that Instance. Resources select the retained
// runtime values required by that Program in generated descriptor order.
struct Projection {
  enum class ResourceSource : Count {
    HostTexture,
    InstanceTexture,
  };

  struct Resource {
    ResourceSource source;
    Count offset;
  };

  const U8* program;
  Count parameters_offset;
  Count parameters_size;
  const Resource* resources;
  Count resource_count;
};

static_assert(__is_trivial(Projection::Resource));
static_assert(__is_trivial(Projection));

}  // namespace Perimortem::Graphics
