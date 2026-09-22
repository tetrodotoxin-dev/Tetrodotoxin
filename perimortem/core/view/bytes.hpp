// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.h"
#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"

namespace Perimortem::Core::View {

// A raw read only view of continuous bytes with no endianness.
//
// Byte views are typically used as strings but this is only valid when the
// encoding is ASCII. The main purpose of a byte view is to guarantee the data
// lie on 1 byte boundries (for instance ASCII is 7 bit aligned at the byte).
// Any optimized vectorization opperations will break UTF 8 strings unless you
// know exactly what you are doing (scanning for an exact byte).
//
// Since the structure of the data is undefined, Bytes data does not
// directly convert to Vector data, however Vector data can be converted
// to Bytes data.
class Bytes {
 public:
  using data_type = U8;

  // Default to empty string.
  constexpr Bytes() : source_block(nullptr), size(0) {}

  constexpr Bytes(const View::Bytes&) = default;

  template <Count N>
  constexpr Bytes(const U8 (&source)[N]) : source_block(&source[0]), size(N) {}

  constexpr Bytes(const U8* source, Count source_size)
      : source_block(source), size(source_size) {}

  // Crossing the C boundary copies the borrowed descriptor. The byte owner
  // still determines its lifetime, including when either view is retained.
  constexpr Bytes(perimortem_view_bytes view)
      : source_block(view.data), size(view.size) {}

  constexpr operator perimortem_view_bytes() const {
    return {source_block, size};
  }

  constexpr auto operator==(const View::Bytes& rhs) const -> Bool {
    return rhs.size == size &&
           Data::compare(source_block, rhs.source_block, size);
  }

  constexpr auto operator!=(const View::Bytes& rhs) const -> Bool {
    return !operator==(rhs);
  }

  constexpr auto operator[](Count index) const -> U8 {
    if (index >= size) [[unlikely]] {
      return U8();
    }

    return source_block[index];
  }

  constexpr auto slice(Count start, Count size = Count(-1)) const
      -> View::Bytes {
    if (start >= get_size()) {
      return View::Bytes();
    }

    return View::Bytes(
        source_block + start, Math::min(size, get_size() - start));
  };

  constexpr auto is_empty() const -> Bool { return size == 0; };
  constexpr auto get_size() const -> Count { return size; };
  constexpr auto get_data() const -> const U8* { return source_block; };

 private:
  const U8* source_block;
  Count size;
};

}  // namespace Perimortem::Core::View
