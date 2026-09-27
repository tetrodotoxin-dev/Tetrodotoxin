// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/source/contracts/documentation.hpp"

namespace Tetrodotoxin::Source {

// Provides an interface for inspecting an Abstract with a textual description.
class Documentation {
 public:
  auto get_interface() const -> Contracts::Documentation;

  constexpr virtual ~Documentation() = default;

  static auto get_empty() -> const Documentation&;

  virtual constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes {
    return {};
  }

  virtual constexpr auto line_count() const -> Count { return 0; }

  virtual constexpr auto is_empty() const -> Bool { return line_count() == 0; }
};

}  // namespace Tetrodotoxin::Source

#define TTX_EMPTY_DOCUMENTATION()                              \
  auto get_documentation() const                               \
      -> const Tetrodotoxin::Source::Documentation& override { \
    return Tetrodotoxin::Source::Documentation::get_empty();   \
  }

#define TTX_DOCUMENTATION(expression)                          \
  constexpr auto get_documentation() const                     \
      -> const Tetrodotoxin::Source::Documentation& override { \
    return expression;                                         \
  }
