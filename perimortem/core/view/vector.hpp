// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/selection.hpp"

namespace Perimortem::Core::View {

// A read only view of continuous data with possible endianness and structure.
//
// Structured data can be converted to Bytes data in order to interperet it
// at a byte level, however this is only valid in memory. To write and read
// binary data as vector data it should be serialized using
// `Perimortem::Serialization::Binary`.
template <typename type>
class Vector {
 public:
  using data_type = type;

  constexpr Vector() = default;
  constexpr Vector(const Vector&) = default;
  constexpr Vector(const data_type* entries, const Count size)
      : source_block(entries), size(size) {}

  template <Count N>
  constexpr Vector(const data_type (&source)[N])
      : source_block(source), size(N) {}

  constexpr auto contains(const data_type& data) const -> Bool {
    for (Count i = 0; i < size; i++) {
      if (source_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  // Tests each value in order and stops when the predicate accepts one.
  template <typename predicate_type>
  constexpr auto contains(predicate_type predicate) const
      -> decltype(Bool(predicate(*static_cast<const data_type*>(nullptr)))) {
    return find(predicate) != -1;
  }

  // Tests each value in order and returns the index of the first item that
  // resolve the predicate or -1 if no item does.
  template <typename predicate_type>
  constexpr auto find(predicate_type predicate) const -> Count {
    for (Count i = 0; i < size; i++) {
      if (predicate(source_block[i])) {
        return i;
      }
    }

    return -1;
  }

  constexpr auto operator==(const Vector& rhs) const -> Bool {
    if (rhs.size != size) {
      return false;
    }

    for (Count i = 0; i < size; i++) {
      if (source_block[i] != rhs.source_block[i]) {
        return false;
      }
    }

    return true;
  }

  constexpr auto operator[](Count index) const -> data_type {
    if (index >= size) [[unlikely]] {
      return data_type();
    }

    return source_block[index];
  }

  constexpr auto slice(Count start, Count size = Count(-1)) const
      -> View::Vector<data_type> {
    if (start >= get_size()) {
      return View::Vector<data_type>();
    }

    return View::Vector<data_type>(
        source_block + start, Math::min(size, get_size() - start));
  };

  constexpr auto is_empty() const -> Bool { return size == 0; };
  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_data() const -> const data_type* { return source_block; }
  constexpr auto get_bytes() const -> const Bytes {
    return Bytes(Data::cast<const U8>(source_block), size * sizeof(data_type));
  }

  constexpr auto begin() const -> Selection<Vector> {
    return Selection<Vector>(*this);
  }

  constexpr auto end() const -> Selection<Vector> {
    return Selection<Vector>(*this, get_size());
  }

 private:
  const type* source_block = nullptr;
  Count size = 0;
};

}  // namespace Perimortem::Core::View
