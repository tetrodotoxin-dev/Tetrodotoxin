// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <stddef.h>

#include "ttx/data/form/schema.hpp"

namespace Ttx::Data::Form {

// Native declarations can supply their Data description without repeating
// each function's argument and result types. Record fields remain explicit:
// C++ has no field reflection, and sizeof alone cannot describe their ABI.
// Specializations supply those fields using their actual offsetof values.
// The resulting Schema goes through the ordinary representation compiler.
//
// TODO: Replace when we actually get C++ reflection in clang. Thank you C++26!
template <typename Type>
class Native {
  static consteval auto describe() -> Schema {
    using Value = Schema::Value;
    if constexpr (__is_same(Type, U8) || __is_same(Type, bool)) {
      return Schema::primitive(Value::U8);
    } else if constexpr (__is_same(Type, U16)) {
      return Schema::primitive(Value::U16);
    } else if constexpr (__is_same(Type, U32)) {
      return Schema::primitive(Value::U32);
    } else if constexpr (__is_same(Type, U64)) {
      return Schema::primitive(Value::U64);
    } else if constexpr (__is_same(Type, S8)) {
      return Schema::primitive(Value::S8);
    } else if constexpr (__is_same(Type, S16)) {
      return Schema::primitive(Value::S16);
    } else if constexpr (__is_same(Type, S32)) {
      return Schema::primitive(Value::S32);
    } else if constexpr (__is_same(Type, S64)) {
      return Schema::primitive(Value::S64);
    } else if constexpr (__is_same(Type, R32)) {
      return Schema::primitive(Value::R32);
    } else if constexpr (__is_same(Type, R64)) {
      return Schema::primitive(Value::R64);
    } else if constexpr (__is_enum(Type)) {
      return Native<__underlying_type(Type)>::schema;
    } else {
      static_assert(
          sizeof(Type) == 0, "Native records need an explicit Schema");
    }
  }

 public:
  static constexpr Schema schema = describe();
  static constexpr Schema::Reference reference = schema;
};

// An array's element type and length establish its repeated storage geometry.
// sizeof includes any element padding, so the Range preserves native spacing.
template <typename Type, Count size>
class Native<Type[size]> {
 public:
  static constexpr Schema schema = Schema::range(
      Native<Type>::reference,
      size,
      sizeof(Type),
      sizeof(Type) * size,
      alignof(Type));
  static constexpr Schema::Reference reference = schema;
};

template <typename Type>
class Native<const Type> : public Native<Type> {};

template <typename Type>
class Native<Type*> {
 private:
  static constexpr auto target() -> const Schema* {
    if constexpr (__is_pointer(Type)) {
      // A pointer to a pointer addresses a stored pointer slot. Its wrapper
      // preserves that extra indirection instead of collapsing two pointers
      // into the same target selector.
      static constexpr Schema::Position field(Native<Type>::reference, 0);
      static constexpr Schema schema =
          Schema::composite({&field, 1}, sizeof(Type), alignof(Type));
      return &schema;
    } else {
      return &Native<Type>::schema;
    }
  }

 public:
  static constexpr Schema::Reference reference = Schema::pointer(target());
};

// A void return has no occupied storage. Its unset reference selects the
// callable's reserved return encoding instead of describing a value.
template <>
class Native<void> {
 public:
  static constexpr Schema::Reference reference = Schema::Reference();
};

template <>
class Native<void*> {
 public:
  static constexpr Schema::Reference reference = Schema::pointer();
};

template <>
class Native<const void*> : public Native<void*> {};

template <typename Result, typename... Arguments>
class Native<Result (*)(Arguments...)> {
  static constexpr Schema::Argument arguments[] = {
    Native<Arguments>::reference..., Schema::Argument()};

 public:
  static constexpr Schema schema = Schema::callable(
      Schema::Convention::Native,
      {arguments, sizeof...(Arguments)},
      Native<Result>::reference);
  static constexpr Schema::Reference reference = schema;
};

template <typename Result, typename... Arguments>
class Native<Result (*)(Arguments..., ...)> {
  static constexpr Schema::Argument arguments[] = {
    Native<Arguments>::reference..., Schema::Argument()};

 public:
  static constexpr Schema schema = Schema::callable(
      Schema::Convention::NativeVariadic,
      {arguments, sizeof...(Arguments)},
      Native<Result>::reference);
  static constexpr Schema::Reference reference = schema;
};

}  // namespace Ttx::Data::Form

// These declarations use real C fields, so renaming a field or changing its
// function signature also changes its description at the same compile boundary.
#define TTX_DATA_MEMBER(type, member)                             \
  Ttx::Data::Form::Schema::Position(                              \
      Ttx::Data::Form::Native<decltype(type::member)>::reference, \
      offsetof(type, member))

#define TTX_DATA_RECORD(type, ...)                                          \
  template <>                                                               \
  class Ttx::Data::Form::Native<type> {                                     \
    static_assert(                                                          \
        __is_standard_layout(type) &&                                       \
            __is_trivially_constructible(type, const type&) &&              \
            __is_trivially_destructible(type),                              \
        "Native API records require a trivial C-compatible call boundary"); \
    static constexpr Schema::Position fields[] = {__VA_ARGS__};             \
                                                                            \
   public:                                                                  \
    static constexpr Schema schema = Schema::composite(                     \
        {fields, sizeof(fields) / sizeof(fields[0])},                       \
        sizeof(type),                                                       \
        alignof(type));                                                     \
    static constexpr Schema::Reference reference = schema;                  \
  }
