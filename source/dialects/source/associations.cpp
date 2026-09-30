// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialects/source/associations.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Dialects;

static constexpr auto contains(Source::Token token, Count offset) -> Bool {
  return token && offset >= token.get_offset() &&
         offset < Count(token.get_offset()) + Count(token.get_size());
}

static constexpr auto contains(Source::Span span, Count offset) -> Bool {
  return span && offset >= span.get_offset() &&
         offset < Count(span.get_offset()) + span.get_size();
}

auto Source::Associations::create(
    Anchor anchor,
    const Ttx::Concept::Abstract& semantic) -> void {
  if (!anchor.get_span()) {
    return;
  }

  associations.insert(Entry(anchor, semantic));
}

auto Source::Associations::find_at(Count offset) const
    -> Option<Ttx::Concept::Abstract> {
  Option<const Associations::Entry&> selected;
  Bool selected_focus = False;
  Count selected_extent = Count(-1);

  auto source_associations = associations.get_view();
  for (Count i = 0; i < source_associations.get_size(); i++) {
    const Associations::Entry& association = source_associations.get_data()[i];
    Source::Anchor anchor = association.get_anchor();
    Source::Token focus = anchor.get_token();
    Source::Span span = anchor.get_span();
    Bool contains_focus = contains(focus, offset);
    Bool contains_span = contains(span, offset);
    if (!contains_focus && !contains_span) {
      continue;
    }

    Count extent = contains_focus ? focus.get_size() : span.get_size();
    if (!selected || (contains_focus && !selected_focus) ||
        (contains_focus == selected_focus && extent < selected_extent)) {
      selected = association;
      selected_focus = contains_focus;
      selected_extent = extent;
    }
  }

  if (!selected) {
    return {};
  }

  return selected->get_semantic();
}

auto Source::Associations::find(const Ttx::Concept::Abstract& semantic) const
    -> Option<Source::Anchor> {
  for (const Entry& association : associations.get_view()) {
    if (association.get_semantic().get_identity() == semantic.get_identity()) {
      return association.get_anchor();
    }
  }

  return {};
}
