// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"

namespace Perimortem::Core::Algorithm {

// Fast vectorized sub string search for View::Bytes.
auto search(View::Bytes src, View::Bytes value) -> Count;

// Fast vectorized sub string search for a particular byte in a View::Bytes.
auto search(View::Bytes src, U8 value) -> Count;

// Returns the index of the smallest element in a View::Vector.
// If multiple elements are the smallest then the lowest index is used.
template <typename element_type>
constexpr auto min_element(View::Vector<element_type> src) -> Count {
  const auto* data = src.get_data();
  Count target_index = 0;
  for (Count i = 1; i < src.get_size(); i++) {
    if (data[i] < data[target_index]) {
      target_index = i;
    }
  }

  return target_index;
}

// Returns the index of the largest element in a View::Vector.
// If multiple elements are the largest then the lowest index is used.
template <typename element_type>
constexpr auto max_element(View::Vector<element_type> src) -> Count {
  const auto* data = src.get_data();
  Count target_index = 0;
  for (Count i = 1; i < src.get_size(); i++) {
    if (data[i] > data[target_index]) {
      target_index = i;
    }
  }

  return target_index;
}

}  // namespace Perimortem::Core::Algorithm
