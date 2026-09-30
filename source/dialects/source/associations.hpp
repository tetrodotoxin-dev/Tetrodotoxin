// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/dialects/source/anchor.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Dialects::Source {

// While a source graph is being built, Associations remembers which authored
// Anchor introduced each semantic identity. It shares the source transaction's
// Arena, so Workspace can keep this index beside the completed graph after the
// mutable Cursor is gone.
//
// Entries copy borrowed Abstract views without acquiring their lifetime. The
// source owner keeps the supplying publication and provider code alive while
// this index is used. Identity is scoped to that publication, not to the C++
// address of a temporary view passed to create or find.
class Associations {
 public:
  class Entry {
   public:
    constexpr Entry(Anchor anchor, const Ttx::Concept::Abstract& semantic)
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

  // An authored Span gives an identity a place in source. Synthetic identities
  // have no such place, so there is nothing useful to add to this index.
  auto create(Anchor anchor, const Ttx::Concept::Abstract& semantic) -> void;

  // An exact Token is the best answer when several authored ranges cover one
  // byte. When only Spans overlap, the narrower range gives editor tooling the
  // most specific identity, and construction order keeps equal ranges stable.
  // The returned view is independent of entry storage, so growing the index
  // cannot invalidate a previous answer. Its provider lifetime stays borrowed.
  auto find_at(Count offset) const
      -> Perimortem::Core::Option<Ttx::Concept::Abstract>;

  // Looking up the same semantic identity returns the Anchor captured while its
  // source transaction built the graph.
  auto find(const Ttx::Concept::Abstract& semantic) const
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

}  // namespace Tetrodotoxin::Dialects::Source
