// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/data.hpp"

namespace Perimortem::Utility {

// Result selects exactly one value or error. The missing default constructor
// and private state leave no empty outcome for a caller to interpret.
//
// Result owns its storage because a successful value may require destruction.
// Errors remain owned trivially destructible values in the first contract, so
// only one alternative needs active cleanup. Borrowed values store one pointer
// and retain the reference identity established by the caller.
//
// Two case visitation makes error handling visible at the call site without an
// unchecked value shortcut that could discard the error branch.
template <typename value_type, typename error_type>
  requires(
      (!__is_lvalue_reference(error_type)) &&
      (!__is_rvalue_reference(error_type)) &&
      __is_trivially_destructible(error_type) &&
      (!__is_rvalue_reference(value_type)) &&
      (!__is_same(__remove_cvref(value_type), __remove_cvref(error_type))))
class Result {
 private:
  template <typename selected_type>
  static auto storage_type() -> selected_type;

  template <typename selected_type>
    requires(__is_lvalue_reference(selected_type))
  static auto storage_type() -> __remove_reference_t(selected_type)*;

  using ValueStorage = decltype(storage_type<value_type>());
  using ValueAlternative = __remove_cvref(value_type);
  using ErrorAlternative = __remove_cvref(error_type);

  static constexpr bool value_requires_destruction =
      !__is_trivially_destructible(ValueStorage);
  template <typename candidate_type>
  static consteval auto value_constructible() -> bool {
    return (!__is_lvalue_reference(value_type) ||
            __is_lvalue_reference(candidate_type)) &&
           __is_constructible(value_type, candidate_type&&);
  }

  template <typename candidate_type>
  static consteval auto error_constructible() -> bool {
    return __is_constructible(error_type, candidate_type&&);
  }

  template <typename candidate_type>
  static consteval auto selects_value() -> bool {
    if constexpr (__is_same(__remove_cvref(candidate_type), ValueAlternative)) {
      return value_constructible<candidate_type>();
    }

    if constexpr (__is_same(__remove_cvref(candidate_type), ErrorAlternative)) {
      return false;
    }

    return value_constructible<candidate_type>() &&
           !error_constructible<candidate_type>();
  }

  template <typename candidate_type>
  static consteval auto selects_error() -> bool {
    if constexpr (__is_same(__remove_cvref(candidate_type), ErrorAlternative)) {
      return error_constructible<candidate_type>();
    }

    if constexpr (__is_same(__remove_cvref(candidate_type), ValueAlternative)) {
      return false;
    }

    return error_constructible<candidate_type>() &&
           !value_constructible<candidate_type>();
  }

  template <typename candidate_type>
  static consteval auto accepts() -> bool {
    return selects_value<candidate_type>() != selects_error<candidate_type>();
  }

  static consteval auto value_copy_constructible() -> bool {
    if constexpr (__is_lvalue_reference(value_type)) {
      return true;
    }

    return __is_constructible(value_type, const value_type&);
  }

  static consteval auto value_move_constructible() -> bool {
    if constexpr (__is_lvalue_reference(value_type)) {
      return true;
    }

    return __is_constructible(value_type, value_type&&);
  }

  static constexpr bool copy_constructible =
      value_copy_constructible() &&
      __is_constructible(error_type, const error_type&);
  static constexpr bool move_constructible =
      value_move_constructible() &&
      __is_constructible(error_type, error_type&&);

  template <typename candidate_type>
  constexpr auto construct_value(candidate_type&& candidate) -> void {
    if constexpr (__is_lvalue_reference(value_type)) {
      auto& reference = static_cast<value_type>(candidate);
      new (&value, Core::Placement::Construct) ValueStorage(&reference);
    } else {
      new (&value, Core::Placement::Construct)
          ValueStorage(static_cast<candidate_type&&>(candidate));
    }

    value_selected = True;
  }

  template <typename candidate_type>
  constexpr auto construct_error(candidate_type&& candidate) -> void {
    new (&error, Core::Placement::Construct)
        error_type(static_cast<candidate_type&&>(candidate));
    value_selected = False;
  }

  constexpr auto get_value() -> decltype(auto) {
    if constexpr (__is_lvalue_reference(value_type)) {
      return *value;
    } else {
      return (value);
    }
  }

  constexpr auto get_value() const -> decltype(auto) {
    if constexpr (__is_lvalue_reference(value_type)) {
      return *value;
    } else {
      return (value);
    }
  }

  constexpr auto get_error() -> error_type& { return error; }

  constexpr auto get_error() const -> const error_type& { return error; }

  constexpr auto destroy_value() -> void {
    if constexpr (value_requires_destruction) {
      if (value_selected) {
        get_value().~ValueAlternative();
      }
    }
  }

  constexpr auto construct_copy(const Result& source) -> void {
    if (source.value_selected) {
      construct_value(source.get_value());
    } else {
      construct_error(source.get_error());
    }
  }

  constexpr auto construct_move(Result& source) -> void {
    if (source.value_selected) {
      if constexpr (__is_lvalue_reference(value_type)) {
        construct_value(source.get_value());
      } else {
        construct_value(Core::Data::take(source.get_value()));
      }
    } else {
      construct_error(Core::Data::take(source.get_error()));
    }
  }

  union {
    ValueStorage value;
    error_type error;
  };
  Bool value_selected;

 public:
  Result() = delete;

  template <typename candidate_type>
    requires(accepts<candidate_type>())
  constexpr Result(candidate_type&& candidate) {
    if constexpr (selects_value<candidate_type>()) {
      construct_value(static_cast<candidate_type&&>(candidate));
    } else {
      construct_error(static_cast<candidate_type&&>(candidate));
    }
  }

  constexpr Result(const Result& source)
    requires(copy_constructible)
  {
    construct_copy(source);
  }

  constexpr Result(const Result&)
    requires(!copy_constructible)
  = delete;

  constexpr Result(Result&& source)
    requires(move_constructible)
  {
    construct_move(source);
  }

  constexpr Result(Result&&)
    requires(!move_constructible)
  = delete;

  constexpr ~Result()
    requires(!value_requires_destruction)
  = default;

  constexpr ~Result()
    requires(value_requires_destruction)
  {
    destroy_value();
  }

  constexpr auto operator=(const Result& source) -> Result&
    requires(copy_constructible)
  {
    if (this == &source) {
      return *this;
    }

    destroy_value();
    construct_copy(source);
    return *this;
  }

  constexpr auto operator=(const Result&) -> Result&
    requires(!copy_constructible)
  = delete;

  constexpr auto operator=(Result&& source) -> Result&
    requires(move_constructible)
  {
    if (this == &source) {
      return *this;
    }

    destroy_value();
    construct_move(source);
    return *this;
  }

  constexpr auto operator=(Result&&) -> Result&
    requires(!move_constructible)
  = delete;

  template <typename value_callback, typename error_callback>
  constexpr auto visit(
      value_callback value_visitor,
      error_callback error_visitor) -> decltype(auto) {
    if (value_selected) {
      return value_visitor(get_value());
    }

    return error_visitor(get_error());
  }

  template <typename value_callback, typename error_callback>
  constexpr auto visit(
      value_callback value_visitor,
      error_callback error_visitor) const -> decltype(auto) {
    if (value_selected) {
      return value_visitor(get_value());
    }

    return error_visitor(get_error());
  }
};

}  // namespace Perimortem::Utility
