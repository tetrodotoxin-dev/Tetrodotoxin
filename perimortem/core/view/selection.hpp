// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Core::View {

// Selection is a read only iterator over the values in any View admitted by
// one predicate. The same value is the range returned to range iteration and
// the iterator returned by begin() and end(). It retains no parallel indices or
// selected storage.
template <
    typename view_type,
    typename predicate_type = Bool (*)(const typename view_type::data_type&)>
class Selection {
 public:
  using data_type = typename view_type::data_type;

  constexpr Selection(view_type source) : Selection(source, accept, 0, True) {}

  constexpr Selection(view_type source, predicate_type predicate)
      : Selection(source, static_cast<predicate_type&&>(predicate), 0, True) {}

  constexpr Selection(view_type source, Count position)
      : Selection(source, accept, position, False) {}

  constexpr auto operator*() const -> data_type {
    return source.get_data()[position];
  }

  constexpr auto operator++() -> Selection& {
    position++;
    select();
    return *this;
  }

  constexpr auto operator!=(const Selection& rhs) const -> Bool {
    return position != rhs.position;
  }

  constexpr auto operator==(const Selection& rhs) const -> Bool {
    return position == rhs.position;
  }

  constexpr auto begin() const -> Selection {
    return Selection(source, predicate, 0, True);
  }

  constexpr auto end() const -> Selection {
    return Selection(source, predicate, source.get_size(), False);
  }

 private:
  static constexpr auto accept(const data_type&) -> Bool { return True; }

  constexpr Selection(
      view_type source,
      predicate_type predicate,
      Count position,
      Bool select_position)
      : source(source),
        predicate(static_cast<predicate_type&&>(predicate)),
        position(position) {
    if (select_position) {
      select();
    }
  }

  constexpr auto select() -> void {
    while (position < source.get_size() &&
           !predicate(source.get_data()[position])) {
      position++;
    }
  }

  view_type source;
  predicate_type predicate;
  Count position;
};

template <typename view_type>
Selection(view_type) -> Selection<view_type>;

template <typename view_type, typename predicate_type>
Selection(view_type, predicate_type) -> Selection<view_type, predicate_type>;

}  // namespace Perimortem::Core::View
