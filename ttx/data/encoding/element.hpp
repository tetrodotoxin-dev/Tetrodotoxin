// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/encoding/block.hpp"
#include "ttx/data/form/schema.hpp"

namespace Ttx::Data::Encoding {

// One element block describes a progression of primitives or references to
// struct or callable bodies. References locate those bodies by absolute block
// indices. The pointer flag selects the buffer's pointer storage while
// retaining the target description. Traversal can therefore stop at pointer
// storage while agreement still compares the complete target format.
struct Element {
  Count count;
  Count offset;
  Count distance;
  Count type;
  Count attributes;

  enum : Count { Pointer = 1, Callable = 2, Struct = 4 };

  constexpr Element(
      Count count = 0,
      Count offset = 0,
      Count distance = 0,
      Count type = 0,
      Count attributes = 0)
      : count(count),
        offset(offset),
        distance(distance),
        type(type),
        attributes(attributes) {}

  constexpr auto operator==(const Element&) const -> bool = default;
  constexpr auto references() const -> Bool {
    return attributes & (Struct | Callable);
  }

  constexpr auto is_inline() const -> Bool {
    return (attributes & (Struct | Pointer)) == Struct;
  }

  constexpr auto is_pointer() const -> Bool { return attributes & Pointer; }
  constexpr auto get_value() const -> Form::Schema::Value {
    return is_pointer() ? Form::Schema::Value::Pointer
                        : Form::Schema::Value(type & 63);
  }

  constexpr auto get_extent(Count pointer_size = sizeof(void*)) const -> Count {
    return is_pointer() ? pointer_size : Form::Schema::get_width(get_value());
  }

  constexpr auto get_byte_order() const -> Form::Schema::ByteOrder {
    return !is_pointer() && (type & 64) ? Form::Schema::ByteOrder::Big
                                        : Form::Schema::ByteOrder::Little;
  }

  constexpr auto encode(U8 depth) const -> Block {
    const Count q = Count(depth) << 3;
    Block block;
    block.insert(type, 0);
    block.insert(attributes & 7, q - 1);
    block.insert(distance, q + 2);
    block.insert(offset, 2 * q);
    block.insert(count, 3 * q);
    return block;
  }

  static constexpr auto decode(
      Perimortem::Core::View::Bytes bytes,
      Count index,
      U8 depth) -> Element {
    const Count q = Count(depth) << 3;
    const Count first = (index * depth) << 5;
    return Element(
        Block::extract(bytes, first + 3 * q, q),
        Block::extract(bytes, first + 2 * q, q),
        Block::extract(bytes, first + q + 2, q - 2),
        Block::extract(bytes, first + 0, q - 1),
        Block::extract(bytes, first + q - 1, 3));
  }
};

static_assert(sizeof(Element) == 5 * sizeof(Count));

}  // namespace Ttx::Data::Encoding
