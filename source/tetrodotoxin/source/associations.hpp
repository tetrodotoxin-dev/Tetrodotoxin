// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/source/anchor.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Source {

// Associations indexes the authored ranges of one source observation. Its
// owner retains the supplying graph publication while entries borrow semantic
// identities. Cursor traversal owns no graph references, so releasing a failed
// interpretation cannot leave entries in an unrelated input Cursor.
class Associations {
 public:
  class Entry {
   public:
    constexpr Entry(Anchor anchor, Ttx::Concept::Abstract semantic)
        : anchor(anchor), semantic(semantic) {}

    constexpr auto get_anchor() const -> Anchor { return anchor; }

    constexpr auto get_semantic() const -> Ttx::Concept::Abstract {
      return semantic;
    }

   private:
    Anchor anchor;
    Ttx::Concept::Abstract semantic;
  };

  constexpr Associations(Perimortem::Memory::Allocator::Arena& arena)
      : associations(arena) {}
  Associations(const Associations&) = delete;

  // Repeating the exact Anchor replaces its provisional semantic with the
  // strongest linked fact. The supplying graph still owns that identity.
  auto create(Anchor anchor, Ttx::Concept::Abstract semantic) -> void;

  // A focus is the best answer when several authored ranges cover one byte.
  // When only extents overlap, the narrower range gives editor tooling the
  // most specific identity, and construction order keeps equal ranges stable.
  auto find_at(Count offset) const
      -> Perimortem::Core::Option<Ttx::Concept::Abstract>;

  // Looking up the same semantic identity returns the Anchor captured while its
  // source transaction built the graph.
  auto find(Ttx::Concept::Abstract semantic) const
      -> Perimortem::Core::Option<Anchor>;

  // Some editor features need to walk every authored identity rather than
  // select one byte. Sharing this same index keeps those features on the
  // graph's real identities and avoids a second tooling catalog.
  constexpr auto get_entries() const -> Perimortem::Core::View::Vector<Entry> {
    return associations.get_view();
  }

 private:
  Perimortem::Memory::Managed::Vector<Entry> associations;
};

}  // namespace Tetrodotoxin::Source
