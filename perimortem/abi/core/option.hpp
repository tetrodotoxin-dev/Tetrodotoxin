// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Abi::Core {

static_assert(sizeof(bool) == sizeof(U8));
static_assert(alignof(bool) == alignof(U8));

// Option is the trivial native carrier for one optional ABI value. The payload
// precedes its selected state exactly as it does in the language value. ABI
// values provide their own lifetime operations, so this carrier never runs a
// constructor, destructor, retain, or release.
template <typename value_type>
class Option {
  static_assert(__is_trivial(value_type));
  static_assert(__is_standard_layout(value_type));

 public:
  static constexpr auto create() -> Option {
    Option result = {};
    return result;
  }

  static constexpr auto create(const value_type& value) -> Option {
    Option result = {};
    result.value = value;
    result.set = true;
    return result;
  }

  constexpr operator bool() const { return set; }

  constexpr auto operator*() -> value_type& { return value; }
  constexpr auto operator*() const -> const value_type& { return value; }
  constexpr auto operator->() -> value_type* { return &value; }
  constexpr auto operator->() const -> const value_type* { return &value; }

  static constexpr auto get_value_offset() -> Count {
    return __builtin_offsetof(Option, value);
  }

  static constexpr auto get_state_offset() -> Count {
    return __builtin_offsetof(Option, set);
  }

 private:
  value_type value;
  bool set;
};

static_assert(__is_trivial(Option<U64>));
static_assert(__is_standard_layout(Option<U64>));
static_assert(Option<U64>::get_value_offset() == 0);
static_assert(Option<U64>::get_state_offset() == sizeof(U64));

}  // namespace Perimortem::Abi::Core
