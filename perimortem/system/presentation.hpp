// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::System {

// Presentation is the native window value shared with a selected renderer.
// Its three word C compatible shape keeps host handles out of public renderer
// signatures while the Kind tells each backend whether it can consume them.
class Presentation {
 public:
  enum class Kind : U64 {
    None,
    Wayland,
  };

  static constexpr auto create(Kind kind, void* host, void* surface)
      -> Presentation {
    return Presentation(kind, host, surface);
  }

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_host() const -> void* { return host; }
  constexpr auto get_surface() const -> void* { return surface; }

  constexpr auto is_valid() const -> Bool {
    return Bool(kind != Kind::None && host && surface);
  }

 private:
  constexpr Presentation(Kind kind, void* host, void* surface)
      : kind(kind), host(host), surface(surface) {}

  Kind kind = Kind::None;
  void* host = nullptr;
  void* surface = nullptr;
};

static_assert(sizeof(Presentation) == sizeof(U64) + sizeof(void*) * 2);
static_assert(__is_trivially_copyable(Presentation));
static_assert(__is_standard_layout(Presentation));

}  // namespace Perimortem::System
