// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"

namespace Perimortem::Core {

// Implementation is the two word native carrier for one explicitly erased
// Object. The Projection is immutable process data derived by the selected ABI
// Terminal. Implementation owns only the real Object lifetime and preserves the
// exact Projection paired with it.
class Implementation {
 public:
  Implementation() = default;
  Implementation(const Implementation& source);
  Implementation(Implementation&& source);
  ~Implementation();

  auto operator=(const Implementation& source) -> Implementation&;
  auto operator=(Implementation&& source) -> Implementation&;

  static auto retain(Object<> object, const void* projection)
      -> Option<Implementation>;

  // ABI results transfer one already owned pair into C++ without adding a
  // reservation. The producing Terminal guarantees that both words describe
  // the same accepted relationship.
  static constexpr auto adopt(Object<> object, const void* projection)
      -> Implementation {
    return Implementation(object, projection);
  }

  constexpr auto is_empty() const -> Bool { return object.is_empty(); }
  constexpr auto is_valid() const -> Bool {
    return !object.is_empty() && projection != nullptr;
  }
  constexpr auto get_object() const -> Object<> { return object; }
  constexpr auto get_projection() const -> const void* { return projection; }

 private:
  constexpr Implementation(Object<> object, const void* projection)
      : object(object), projection(projection) {}

  Object<> object;
  const void* projection = nullptr;
};

static_assert(sizeof(Implementation) == sizeof(U8*) * 2);
static_assert(alignof(Implementation) == alignof(U8*));
static_assert(__is_standard_layout(Implementation));

}  // namespace Perimortem::Core
