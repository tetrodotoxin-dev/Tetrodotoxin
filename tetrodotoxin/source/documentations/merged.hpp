// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Source::Documentations {

// Merges two documentation sources into one continuation.
// Multiple merges can be stacked together to create a documentation chain,
// however chaining should not be used as a "multi line" comment abstraction.
// For multi line comments attached to a single source use `Block` instead.
class Merged : public Tetrodotoxin::Source::Documentation {
 public:
  constexpr Merged(
      const Tetrodotoxin::Source::Documentation& first,
      const Tetrodotoxin::Source::Documentation& second)
      : first(first), second(second) {}

  constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes override {
    const Count first_count = first.line_count();
    if (index < first_count) {
      return first.get_line(index);
    }

    return second.get_line(index - first_count);
  }

  constexpr auto line_count() const -> Count override {
    return first.line_count() + second.line_count();
  }

 private:
  const Tetrodotoxin::Source::Documentation& first;
  const Tetrodotoxin::Source::Documentation& second;
};

}  // namespace Tetrodotoxin::Source::Documentations
