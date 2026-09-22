// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/perimortem.hpp"

#pragma once

namespace Perimortem::Core::Math {

template <typename type>
constexpr auto max(type left, type right) -> type {
  return left > right ? left : right;
}

template <typename type>
constexpr auto min(type left, type right) -> type {
  return left < right ? left : right;
}

template <typename type>
constexpr auto clamp(type value, type min_value, type max_value) -> type {
  return max(min(value, max_value), min_value);
}

template <typename type>
constexpr auto wrap(type value, type modulo) -> type {
  value %= modulo;
  return value < 0 ? value + modulo : value;
}

template <typename type>
constexpr auto absolute(type value) -> type {
  return value >= 0 ? value : -value;
}

// Full width values skip the terminal shift because C++ does not define a
// shift by the size of the value. Smaller widths compare against the first
// excluded value which avoids manufacturing host minimum and maximum values.
constexpr auto is_representable(S64 value, Count byte_width) -> Bool {
  if (byte_width == 0 || byte_width > sizeof(S64)) {
    return False;
  }

  if (byte_width == sizeof(S64)) {
    return True;
  }

  S64 limit = S64(1) << (byte_width * 8 - 1);
  return value >= -limit && value < limit;
}

constexpr auto is_representable(U64 value, Count byte_width) -> Bool {
  if (byte_width == 0 || byte_width > sizeof(U64)) {
    return False;
  }

  if (byte_width == sizeof(U64)) {
    return True;
  }

  U64 limit = U64(1) << (byte_width * 8);
  return value < limit;
}

constexpr auto log2(U64 value) -> U64 {
  return 64 - __builtin_clzg(value, S32(sizeof(U64) * 8));
}

constexpr auto sqrt(R64 value) -> R64 {
  return __builtin_sqrt(value);
}

constexpr auto floor(R64 value) -> Count {
  return Count(__builtin_floor(value));
}

constexpr auto ceil(R64 value) -> Count {
  return Count(__builtin_ceil(value));
}

}  // namespace Perimortem::Core::Math
