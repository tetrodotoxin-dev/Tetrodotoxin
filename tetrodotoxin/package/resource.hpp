// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/resource.hpp"

namespace Tetrodotoxin::Package {

// Resource is one Package owned immutable file. Its normalized logical route
// is both the semantic name shared by every consumer and the durable key used
// by Archives and target products. Keeping that identity beside the retained
// bytes lets several source members borrow one value without manufacturing
// another resource model for Library, App, or a Terminal.
class Resource : public Tetrodotoxin::Language::Resource {
 public:
  TTX_CONTRACT(Resource, Tetrodotoxin::Language::Resource);

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::View::Bytes value) -> Resource&;

  TTX_NAME(route);

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

  constexpr auto get_value() const -> Perimortem::Core::View::Bytes override {
    return value;
  }

 private:
  constexpr Resource(
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::View::Bytes value)
      : route(route), value(value) {}

  Perimortem::Core::View::Bytes route;
  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Package
