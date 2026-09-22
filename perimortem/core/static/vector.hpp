// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/vector.hpp"

namespace Perimortem::Core::Static {

// Typically used for small value type arrays.
template <typename type, Count literal_size>
class Vector {
 private:
  struct Storage {
    type source_block[literal_size];
  };

  Storage storage{};

 public:
  static constexpr Count size = literal_size;

  constexpr Vector() {}

  // Static vectors initialize their complete storage from one aggregate
  // source. Call sites use an extra pair of braces to create that source, as in
  // `Static::Vector<U8, 3> values = {{1, 2, 3}}`.
  //
  // Tables of aggregate elements spell the first value's type so the outer
  // list resolves to Storage before C++ elides the remaining element braces.
  //
  // Storage's generated copy constructor constructs every array element in
  // place. This keeps initialization independent of the element count without
  // requiring elements to support default construction or assignment.
  constexpr Vector(const Storage& source) : storage(source) {}

  // Allows for generating data that would be a pain to manually write out.
  constexpr Vector(type (*generator)(Count)) {
    for (Count i = 0; i < literal_size; i++) {
      storage.source_block[i] = generator(i);
    }
  }

  constexpr operator Core::View::Vector<type>() const { return get_view(); }
  constexpr operator Core::Access::Vector<type>() { return get_access(); }

  constexpr auto operator==(const View::Vector<type>& rhs) -> Bool {
    if (rhs.get_size() != literal_size) {
      return False;
    }

    return Data::compare(storage.source_block, rhs.get_data(), literal_size);
  }

  constexpr auto operator!=(const View::Vector<type>& rhs) -> Bool {
    return !(*this == rhs);
  }

  constexpr auto operator[](Count index) -> type& {
    return storage.source_block[index];
  }

  constexpr auto operator[](Count index) const -> const type& {
    return storage.source_block[index];
  }

  constexpr auto slice(Count start, Count size = Count(-1)) const
      -> Core::View::Vector<type> {
    return get_view().slice(start, size);
  }

  constexpr auto is_empty() const -> Bool { return literal_size == 0; };
  constexpr auto get_size() const -> Count { return literal_size; }
  constexpr auto get_capacity() const -> Count { return literal_size; }
  constexpr auto get_view() const -> const Core::View::Vector<type> {
    return Core::View::Vector<type>(storage.source_block, literal_size);
  }

  constexpr auto get_data() const -> const type* {
    return storage.source_block;
  }
  constexpr auto get_data() -> type* { return storage.source_block; }
  constexpr auto get_access() -> Core::Access::Vector<type> {
    return Core::Access::Vector<type>(storage.source_block, literal_size);
  }
};

}  // namespace Perimortem::Core::Static
