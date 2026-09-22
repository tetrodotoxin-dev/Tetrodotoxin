// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

namespace Tetrodotoxin::Diagnostics {

// Stateless spelling suggestions for failed semantic name lookups.
//
// Exact lookup remains with the semantic owner. These operations borrow the
// complete candidate set for one selection transaction and return a formatted
// hint without retaining candidate or result state. Candidate order is
// preference order: the first exact or one edit name returns immediately,
// while the first two edit name is only a fallback.
class Suggestions {
 public:
  static auto possible_candidate(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> candidates)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Tetrodotoxin::Diagnostics
