// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/data.hpp"

namespace Perimortem::Core::Static {

// A tagged union type that allows null tagging to represent no value.
// Each possible type must be unique. Value alternatives are managed using byte
// laundering, while reference alternatives store one non owning pointer and
// preserve the referred object's identity. A reference can only be constructed
// from an lvalue, so the Union cannot retain a temporary through const binding.
// Destructable alternatives aren't supported.
template <typename... value_types>
class Union {
 private:
  static_assert(
      sizeof...(value_types) != 0,
      "Union requires at least one provided type.");
  static_assert(
      sizeof...(value_types) < 255,
      "Union supports at most 254 types plus null.");

  template <typename value_type>
  static consteval auto type_count() -> Count {
    return (Count(__is_same(value_type, value_types)) + ...);
  }

  static_assert(
      ((type_count<value_types>() == 1) && ...),
      "All provided Union types must be unique.");
  static_assert(
      (__is_trivially_destructible(value_types) && ...),
      "All provided Union types must be trivially destructible.");

  template <typename value_type>
  static auto storage_type() -> value_type;

  template <typename value_type>
    requires(__is_lvalue_reference(value_type))
  static auto storage_type() -> __remove_reference_t(value_type)*;

  template <typename value_type>
  using Storage = decltype(storage_type<value_type>());

  static consteval auto storage_size() -> Count {
    Count size = 0;
    ((size = size < sizeof(Storage<value_types>) ? sizeof(Storage<value_types>)
                                                 : size),
     ...);
    return size;
  }

  static consteval auto storage_alignment() -> Count {
    Count alignment = 0;
    ((alignment = alignment < alignof(Storage<value_types>)
                      ? alignof(Storage<value_types>)
                      : alignment),
     ...);
    return alignment;
  }

  template <typename value_type>
  static consteval auto type_tag() -> U8 {
    U8 result = 0;
    U8 candidate = 1;
    ((result = __is_same(value_type, value_types) ? candidate : result,
      candidate++),
     ...);
    return result;
  }

  template <typename value_type>
  using Alternative = __remove_cvref(value_type);

  template <typename Candidate, typename value_type>
  static consteval auto constructible() -> bool {
    return (!__is_lvalue_reference(value_type) ||
            __is_lvalue_reference(Candidate)) &&
           __is_constructible(value_type, Candidate&&);
  }

  template <typename Candidate>
  static consteval auto constructible_count() -> Count {
    return (Count(constructible<Candidate, value_types>()) + ...);
  }

  template <typename Candidate, typename value_type>
  static consteval auto selects() -> bool {
    // An exact alternative always wins. Otherwise the source must construct
    // exactly one alternative, allowing `Union<U64>` to accept an
    // integer literal without making a multiple numeric Union guess its
    // intended type.
    constexpr Count exact_reference = type_count<Candidate>();
    if constexpr (exact_reference != 0) {
      return __is_same(Candidate, value_type) &&
             constructible<Candidate, value_type>();
    }

    constexpr Count exact_value = type_count<Alternative<Candidate>>();
    if constexpr (exact_value != 0) {
      return __is_same(Alternative<Candidate>, value_type) &&
             constructible<Candidate, value_type>();
    }

    return constructible_count<Candidate>() == 1 &&
           constructible<Candidate, value_type>();
  }

  template <typename Candidate>
  static consteval auto accepts() -> bool {
    return (Count(selects<Candidate, value_types>()) + ...) == 1;
  }

  template <typename value_type, typename Candidate>
  constexpr auto construct(Candidate&& candidate) -> decltype(auto) {
    if constexpr (__is_lvalue_reference(value_type)) {
      auto& reference = static_cast<value_type>(candidate);
      new (storage, Placement::Construct) Storage<value_type>(&reference);
      tag = type_tag<value_type>();
      return reference;
    } else {
      value_type& value = *new (storage, Placement::Construct)
                              value_type(static_cast<Candidate&&>(candidate));
      tag = type_tag<value_type>();
      return value;
    }
  }

  template <typename value_type, typename... Rest, typename Candidate>
  constexpr auto construct_candidate(Candidate&& value) -> void {
    if constexpr (selects<Candidate, value_type>()) {
      construct<value_type>(static_cast<Candidate&&>(value));
    } else if constexpr (sizeof...(Rest) != 0) {
      construct_candidate<Rest...>(static_cast<Candidate&&>(value));
    } else {
      static_assert(
          selects<Candidate, value_type>(),
          "Union construction candidate does not select a supported type.");
    }
  }

  template <typename value_type>
  constexpr auto active() -> decltype(auto) {
    if constexpr (__is_lvalue_reference(value_type)) {
      return **Data::cast<Storage<value_type>>(storage);
    } else {
      return *Data::cast<value_type>(storage);
    }
  }

  template <typename value_type>
  constexpr auto active() const -> decltype(auto) {
    if constexpr (__is_lvalue_reference(value_type)) {
      return **Data::cast<Storage<value_type>>(storage);
    } else {
      return *Data::cast<const value_type>(storage);
    }
  }

  template <typename value_type, typename... Rest, typename Source>
  constexpr auto construct_active(Source& source) -> void {
    if (source.tag == type_tag<value_type>()) {
      if constexpr (
          __is_same(Source, const Union) || __is_lvalue_reference(value_type)) {
        construct<value_type>(source.template active<value_type>());
      } else {
        construct<value_type>(Data::take(source.template active<value_type>()));
      }
      return;
    }

    if constexpr (sizeof...(Rest) != 0) {
      construct_active<Rest...>(source);
    }
  }

  template <typename value_type, typename... Rest>
  constexpr auto equals_active(const Union& rhs) const -> Bool {
    if (tag == type_tag<value_type>()) {
      if constexpr (__is_lvalue_reference(value_type)) {
        return &active<value_type>() == &rhs.template active<value_type>();
      } else {
        return active<value_type>() == rhs.template active<value_type>();
      }
    }

    if constexpr (sizeof...(Rest) != 0) {
      return equals_active<Rest...>(rhs);
    }

    __builtin_unreachable();
  }

  template <
      typename value_type,
      typename... Rest,
      typename Self,
      typename Visitor>
  constexpr static auto visit_active(Self& self, Visitor& visitor)
      -> decltype(auto) {
    if (self.tag == type_tag<value_type>()) {
      return visitor(self.template active<value_type>());
    }

    if constexpr (sizeof...(Rest) != 0) {
      return visit_active<Rest...>(self, visitor);
    }

    __builtin_unreachable();
  }

  template <typename Self, typename... Cases>
  constexpr static auto dispatch(Self& self, Cases... cases) -> decltype(auto) {
    struct Visitor : Cases... {
      using Cases::operator()...;
    } visitor{static_cast<Cases&&>(cases)...};
    if (self.is_null()) {
      return visitor();
    }

    return visit_active<value_types...>(self, visitor);
  }

  alignas(storage_alignment()) U8 storage[storage_size()];
  U8 tag = 0;

 public:
  constexpr Union() = default;

  template <typename Candidate>
    requires(accepts<Candidate>())
  constexpr Union(Candidate&& value) {
    construct_candidate<value_types...>(static_cast<Candidate&&>(value));
  }

  constexpr Union(const Union& source) {
    if (!source.is_null()) {
      construct_active<value_types...>(source);
    }
  }

  constexpr Union(Union&& source) {
    if (!source.is_null()) {
      construct_active<value_types...>(source);
    }
  }

  constexpr auto operator=(const Union& source) -> Union& {
    if (this == &source) {
      return *this;
    }

    tag = 0;
    if (!source.is_null()) {
      construct_active<value_types...>(source);
    }
    return *this;
  }

  constexpr auto operator=(Union&& source) -> Union& {
    if (this == &source) {
      return *this;
    }

    tag = 0;
    if (!source.is_null()) {
      construct_active<value_types...>(source);
    }
    return *this;
  }

  constexpr auto operator==(const Union& rhs) const -> Bool {
    if (tag != rhs.tag) {
      return False;
    }

    return is_null() ? True : equals_active<value_types...>(rhs);
  }

  constexpr auto operator!=(const Union& rhs) const -> Bool {
    return !(*this == rhs);
  }

  template <typename value_type>
  constexpr auto is() const -> Bool {
    static_assert(
        type_count<value_type>() == 1,
        "Requested value type is not a Union alternative.");
    return tag == type_tag<value_type>();
  }

  template <typename value_type>
  constexpr auto find() {
    static_assert(
        type_count<value_type>() == 1,
        "Requested value type is not a Union alternative.");
    return is<value_type>() ? &active<value_type>() : nullptr;
  }

  template <typename value_type>
  constexpr auto find() const {
    static_assert(
        type_count<value_type>() == 1,
        "Requested value type is not a Union alternative.");
    return is<value_type>() ? &active<value_type>() : nullptr;
  }

  template <typename... Cases>
  constexpr auto visit(Cases... cases) -> decltype(auto) {
    return dispatch(*this, static_cast<Cases&&>(cases)...);
  }

  template <typename... Cases>
  constexpr auto visit(Cases... cases) const -> decltype(auto) {
    return dispatch(*this, static_cast<Cases&&>(cases)...);
  }

  constexpr auto is_null() const -> Bool { return tag == 0; }
};

}  // namespace Perimortem::Core::Static
