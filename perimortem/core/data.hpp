// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

// Provide the blessed memory operations so C++ compilers can properly optimize.
extern "C" {
typedef CppSize size_t;
extern void* memcpy(
    void* __restrict dest,
    const void* __restrict src,
    size_t count) noexcept(true);

extern void* memmove(void* dest, const void* src, size_t count) noexcept(true);

extern int memcmp(const void* a, const void* b, size_t count) noexcept(true);

extern void* memset(void* a, S32 value, size_t count) noexcept(true);
}  // extern "C"

namespace Perimortem::Core::Data {

enum class ByteOrder {
  Little = __ORDER_LITTLE_ENDIAN__,
  Big = __ORDER_BIG_ENDIAN__,
  Native = __BYTE_ORDER__,
};

// Type helpers
template <typename storage>
consteval auto size_in_bits() -> U64 {
  return sizeof(storage) * 8;
}

template <typename type, Count size>
consteval auto array_size(const type (&)[size]) -> Count {
  return size;
}

// Aligns data by moving to the next valid offset of alignment_size.
template <Count alignment_size>
constexpr auto align(Count offset) -> Count {
  if constexpr (alignment_size <= 1) {
    return offset;
  }

  constexpr Count alignment_filter = alignment_size - 1;
  // Check if alignment_size is a power of two, if it is we can avoid a modulo.
  // Any easy test for only a single set bit is to subtract one from the value
  // and and it with itself. It will return nonzero for any non power of two.
  //
  // This technically let's zero through but we special case zero and one.
  if constexpr ((alignment_size & alignment_filter) == 0) {
    const auto required_alignment = (~offset + 1) & (alignment_filter);
    return offset + required_alignment;
  } else {
    const auto required_alignment = alignment_size - (offset % alignment_size);
    return required_alignment == alignment_size ? offset
                                                : offset + required_alignment;
  }
}

// Used for reading const raw data.
template <typename target_type>
auto cast(const void* source) -> const target_type* {
  // TODO: start_lifetime_as
  return reinterpret_cast<const target_type*>(source);
}

// Used for reading raw data.
template <typename target_type>
auto cast(void* source) -> target_type* {
  // TODO: start_lifetime_as
  return reinterpret_cast<target_type*>(source);
}

template <typename storage_type>
auto copy(U8* dest, const storage_type* src, Count count = 1) -> storage_type* {
  return reinterpret_cast<storage_type*>(
      memcpy(dest, src, U64(sizeof(storage_type)) * count));
}

template <typename storage_type>
auto copy(U8* dest, storage_type src) -> storage_type* {
  return reinterpret_cast<storage_type*>(
      memcpy(dest, &src, U64(sizeof(storage_type))));
}

inline auto set(U8* dest, U8 value, Count count = 1) -> void {
  memset(dest, value, count);
}

template <typename storage_type>
auto take(storage_type& source) -> storage_type&& {
  return static_cast<storage_type&&>(source);
}

template <typename storage_type>
constexpr auto
    compare(const storage_type* dest, const storage_type* src, Count count = 1)
        -> Bool {
  if consteval {
    for (Count i = 0; i < count; i++) {
      if (dest[i] != src[i]) {
        return false;
      }
    }

    return true;
  } else {
    return memcmp(dest, src, U64(sizeof(storage_type)) * count) == 0;
  }
}

template <ByteOrder source, ByteOrder target, typename storage_type>
constexpr auto ensure_endian(storage_type value) -> storage_type {
  if constexpr (source != target) {
    switch (sizeof(storage_type)) {
    case 1:
      return value;
    case 2:
      return __builtin_bswap16(value);
    case 4:
      return __builtin_bswap32(value);
    case 8:
      return __builtin_bswap64(value);
    }
  }

  return value;
}

template <ByteOrder target, typename storage_type>
constexpr auto write(storage_type* dest, storage_type value) -> void {
  value = Data::ensure_endian<Data::ByteOrder::Native, target>(value);
  Data::copy(reinterpret_cast<U8*>(dest), &value, 1);
}

// Swaps two objects using laundering mechanics to avoid intermediary objects.
// For consteval move constructors are used.
// For non consteval a forgetful stack buffer is used to avoid constructing and
// destructing a temporary or calling any move constructors.
template <typename type>
constexpr auto swap(type& a, type& b) -> void {
  if !consteval {
    alignas(alignof(type)) U8 forgetful[sizeof(type)];
    memcpy(&forgetful, &a, sizeof(type));
    memcpy((void*)&a, &b, sizeof(type));
    memcpy((void*)&b, &forgetful, sizeof(type));
  } else {
    auto temp = a;
    a = b;
    b = temp;
  }
}

// Launder gets around the C++ object model and allows us to copy underlying
// representations when appropriate.
//
// Use with _caution_. All usages of launder are tracked during reivew.
template <typename type>
auto launder(type& a, const type& b) -> void {
  memcpy((void*)&a, (void*)&b, sizeof(type));
}

}  // namespace Perimortem::Core::Data
