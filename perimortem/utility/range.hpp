// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Utility {

// A half open interval described by its first index and element count.
struct Range {
  Count start = 0;
  Count size = 0;

  constexpr auto get_end() const -> Count { return start + size; }

  constexpr auto is_empty() const -> Bool { return size == 0; }
  constexpr auto has_overlap(Range other) const -> Bool {
    return !is_empty() && !other.is_empty() && start < other.get_end() &&
           other.start < get_end();
  }

  // Expands the range interval to include an index if it's not already covered
  // by the range.
  // If the range is empty then it creates a single element range at the index.
  constexpr auto extend(Count index) -> void {
    if (is_empty()) {
      start = index;
      size = 1;
    } else if (index < start) {
      size += start - index;
      start = index;
    } else if (index >= get_end()) {
      size = index - start + 1;
    }
  }
};

}  // namespace Perimortem::Utility
