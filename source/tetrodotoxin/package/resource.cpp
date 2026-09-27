// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/resource.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

auto Package::Resource::create(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes route,
    Core::View::Bytes value) -> Resource& {
  Core::View::Bytes retained_route = arena.proxy(route);
  Core::View::Bytes retained_value = arena.proxy(value);
  return arena.construct_from<Resource>(
      [&]() -> Resource { return Resource(retained_route, retained_value); });
}
