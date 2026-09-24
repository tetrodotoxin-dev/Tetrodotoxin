// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/data/form/schema.h"

namespace Ttx::Data::Form {

// C++ constructors and accessors belong to the C descriptor record itself.
// Using one type lets both languages borrow descriptor arrays without copying
// or casting wrapper objects. Runtime and constexpr construction describe the
// same source facts.
using Schema = ttx_schema;

}  // namespace Ttx::Data::Form

constexpr auto ttx_schema_callable::get_convention() const {
  return ttx_schema::Convention(convention);
}

constexpr auto ttx_schema::get_width(Value type, Count pointer_size) -> Count {
  switch (type) {
  case Value::U8:
  case Value::S8:
    return 1;
  case Value::U16:
  case Value::S16:
    return 2;
  case Value::U32:
  case Value::S32:
  case Value::R32:
    return 4;
  case Value::U64:
  case Value::S64:
  case Value::R64:
  case Value::V64:
    return 8;
  case Value::Pointer:
    return pointer_size;
  case Value::V128:
    return 16;
  case Value::V256:
    return 32;
  case Value::V512:
    return 64;
  }

  return 0;
}

constexpr auto ttx_schema::primitive(
    Value type,
    ByteOrder order,
    Count pointer_size) -> ttx_schema {
  const Count width = get_width(type, pointer_size);
  return {
    width,
    width,
    static_cast<U8>(Kind::Value),
    {.value = {static_cast<U8>(type), static_cast<U8>(order)}}};
}

constexpr auto ttx_schema::pointer(const ttx_schema* target) -> Reference {
  return Reference(target, TTX_SCHEMA_REFERENCE_POINTER);
}

constexpr auto ttx_schema::callable(
    Convention convention,
    Perimortem::Core::View::Vector<Argument> arguments,
    Reference result,
    Count pointer_size) -> ttx_schema {
  return {
    pointer_size,
    pointer_size,
    static_cast<U8>(Kind::Callable),
    {.callable = {
       arguments.get_data(), arguments.get_size(), result,
       static_cast<ttx_schema_convention>(convention)}}};
}

constexpr auto ttx_schema::composite(
    Perimortem::Core::View::Vector<Position> positions,
    Count extent,
    Count alignment) -> ttx_schema {
  return {
    extent,
    alignment,
    static_cast<U8>(Kind::Composite),
    {.composite = {positions.get_data(), positions.get_size()}}};
}

constexpr auto ttx_schema::range(
    Reference element,
    Count repeats,
    Count distance,
    Count extent,
    Count alignment) -> ttx_schema {
  return {
    extent,
    alignment,
    static_cast<U8>(Kind::Range),
    {.range = {element, repeats, distance}}};
}

constexpr auto ttx_schema_reference::get_extent(Count pointer_size) const
    -> Count {
  if (is_pointer() ||
      (schema && schema->get_kind() == ttx_schema::Kind::Callable)) {
    return pointer_size;
  }

  return schema ? schema->get_extent() : 0;
}

constexpr auto ttx_schema_reference::get_alignment(Count pointer_size) const
    -> Count {
  if (is_pointer() ||
      (schema && schema->get_kind() == ttx_schema::Kind::Callable)) {
    return pointer_size;
  }

  return schema ? schema->get_alignment() : 1;
}
