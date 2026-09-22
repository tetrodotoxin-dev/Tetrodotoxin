// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"

namespace Perimortem::Core::NullTerminated {

inline auto to_view(const char* str) -> Perimortem::Core::View::Bytes {
  if (!str) {
    return Perimortem::Core::View::Bytes();
  }

  constexpr auto length_cutoff = 1 << 10;
  Count size = 0;
  while (str[size] != 0 && size < length_cutoff) {
    size++;
  }

  if (size > length_cutoff) {
    return Perimortem::Core::View::Bytes();
  }

  return Perimortem::Core::View::Bytes(Data::cast<const U8>(str), size);
}

template <CppSize size_including_null>
struct CString {
  U8 content[size_including_null - 1]{};

  consteval CString(const char (&source)[size_including_null]) {
    for (Count i = 0; i < get_size(); i++) {
      content[i] = U8(source[i]);
    }
  }

  consteval operator Core::View::Bytes() const { return get_view(); }

  consteval auto get_size() const -> Count { return size_including_null - 1; }
  consteval auto get_data() const -> const U8* { return content; }
  consteval auto get_view() const -> const Core::View::Bytes {
    return Core::View::Bytes(content, get_size());
  }
};

template <CppSize N>
CString(const char (&)[N]) -> CString<N>;

}  // namespace Perimortem::Core::NullTerminated

// Converts C++ string literals into valid Perimortem byte strings.
template <Perimortem::Core::NullTerminated::CString c_string>
consteval const Perimortem::Core::View::Bytes operator""_view() {
  return c_string.get_view();
}

template <Perimortem::Core::NullTerminated::CString c_string>
consteval const Perimortem::Core::Static::Bytes<c_string.get_size()>
    operator""_bytes() {
  return Perimortem::Core::Static::Bytes<c_string.get_size()>(
      c_string.get_view());
}
