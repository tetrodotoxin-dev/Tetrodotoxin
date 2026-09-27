// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/associations.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;

static auto same(Range left, Range right) -> Bool {
  return left.get_offset() == right.get_offset() &&
         left.get_size() == right.get_size();
}

static auto contains(Range range, Count offset) -> Bool {
  return offset >= range.get_offset() &&
         offset - range.get_offset() < range.get_size();
}

auto Associations::create(Anchor anchor, Ttx::Concept::Abstract semantic)
    -> void {
  for (Count i = 0; i < associations.get_size(); ++i) {
    const auto retained = associations[i].get_anchor();
    const auto first = retained.get_focus();
    const auto second = anchor.get_focus();
    const Bool same_focus =
        Bool(first) == Bool(second) && (!first || same(*first, *second));
    if (retained.get_source() == anchor.get_source() &&
        same(retained.get_extent(), anchor.get_extent()) && same_focus) {
      associations[i] = Entry(anchor, semantic);
      return;
    }
  }

  associations.insert(Entry(anchor, semantic));
}

auto Associations::find_at(Count offset) const
    -> Option<Ttx::Concept::Abstract> {
  Option<Ttx::Concept::Abstract> selected;
  Bool focused = False;
  Count size = Count(-1);
  for (const auto& entry : associations.get_view()) {
    const auto anchor = entry.get_anchor();
    const auto focus = anchor.get_focus();
    const Bool in_focus = focus && contains(*focus, offset);
    if (!in_focus && !contains(anchor.get_extent(), offset)) {
      continue;
    }

    const Count extent =
        in_focus ? focus->get_size() : anchor.get_extent().get_size();
    if (!selected || (in_focus && !focused) ||
        (in_focus == focused && extent < size)) {
      selected = entry.get_semantic();
      focused = in_focus;
      size = extent;
    }
  }

  return selected;
}

auto Associations::find(Ttx::Concept::Abstract semantic) const
    -> Option<Anchor> {
  for (const auto& entry : associations.get_view()) {
    if (entry.get_semantic() == semantic) {
      return entry.get_anchor();
    }
  }

  return {};
}
