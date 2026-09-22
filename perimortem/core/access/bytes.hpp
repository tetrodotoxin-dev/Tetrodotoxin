// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/option.hpp"

namespace Perimortem::Core::Access {

// A raw read/write view of bytes with no endianness.
class Bytes {
 public:
  using data_type = U8;

  // Default to empty string.
  constexpr Bytes() : source_block(nullptr), size(0) {}

  constexpr Bytes(const Bytes&) = default;

  constexpr Bytes(data_type* source, Count source_size)
      : source_block(source), size(source_size) {}

  template <Count N>
  constexpr Bytes(data_type (&source)[N]) : source_block(source), size(N) {}

  constexpr operator View::Bytes() const { return get_view(); }

  constexpr auto at(Count index) -> Core::Option<data_type&> {
    if (index >= size) [[unlikely]] {
      return {};
    }

    return source_block[index];
  }

  constexpr auto operator[](Count index) -> Core::Option<data_type&> {
    return at(index);
  }

  constexpr auto slice(Count start, Count size = Count(-1)) const
      -> Access::Bytes {
    if (start >= get_size()) {
      return Access::Bytes();
    }

    return Access::Bytes(
        source_block + start, Math::min(size, get_size() - start));
  };

  constexpr auto is_empty() const -> Bool { return size == 0; };
  constexpr auto get_size() const -> Count { return size; };
  constexpr auto get_data() -> data_type* { return source_block; };
  constexpr auto get_data() const -> const data_type* { return source_block; };
  constexpr auto get_view() const -> const View::Bytes {
    return View::Bytes(source_block, get_size());
  }

 private:
  data_type* source_block;
  Count size;
};

}  // namespace Perimortem::Core::Access
