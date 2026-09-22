// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/object.hpp"

namespace Perimortem::Memory::Dynamic {

// Record adapts the Core Object carrier to C++ object lifetime rules. It is a
// worker local convenience owner for compiler and tooling state, not a language
// Type or another ABI representation.
template <typename value_type>
class Record {
 public:
  template <typename... arg_types>
  Record(arg_types&&... args) : object(Core::Object<>::create(descriptor)) {
    new (object.get_payload(), Core::Placement::Construct)
        value_type(static_cast<arg_types&&>(args)...);
  }

  Record(Record& rhs) : object(rhs.object) { object.retain(); }
  Record(const Record& rhs) : object(rhs.object) { object.retain(); }
  Record(Record&& rhs) : Record(rhs) {}

  auto operator=(const Record& rhs) -> Record& {
    if (object.get_payload() == rhs.object.get_payload()) {
      return *this;
    }

    object.release();
    object = rhs.object;
    object.retain();
    return *this;
  }

  auto operator=(Record&& rhs) -> Record& {
    if (this == &rhs) {
      return *this;
    }

    Core::Data::swap(object, rhs.object);
    return *this;
  }

  ~Record() { object.release(); }

  constexpr auto operator->() -> value_type* { return get_value(); }
  constexpr auto operator->() const -> const value_type* { return get_value(); }
  constexpr auto operator*() -> value_type& { return *get_value(); }
  constexpr auto operator*() const -> const value_type& { return *get_value(); }

 private:
  static auto destroy(U8* payload) -> void {
    Core::Data::cast<value_type>(payload)->~value_type();
  }

  constexpr auto get_value() const -> value_type* {
    return Core::Data::cast<value_type>(object.get_payload());
  }

  inline static constexpr Core::Object<>::Descriptor descriptor{
    sizeof(value_type), alignof(value_type), destroy};

  Core::Object<> object;
};

static_assert(sizeof(Record<U8>) == sizeof(Core::Object<>));

}  // namespace Perimortem::Memory::Dynamic
