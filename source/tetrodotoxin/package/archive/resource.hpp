// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Package::Archive {

// Resource is the source free value of one Package owned resource identity.
// The logical route remains the durable key, while the bytes let Workspace
// rebuild the same semantic Resource before any member payload is restored.
class Resource {
 public:
  constexpr Resource(
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::View::Bytes value)
      : route(route), value(value) {}

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

  constexpr auto get_value() const -> Perimortem::Core::View::Bytes {
    return value;
  }

 private:
  Perimortem::Core::View::Bytes route;
  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Package::Archive
