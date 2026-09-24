// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/encoding/element.hpp"
#include "ttx/data/encoding/struct.hpp"
#include "ttx/data/form/native.hpp"
#include "ttx/data/form/representation.h"

namespace Ttx::Data::Form {

// C++ navigation uses the C record directly, so either language can borrow a
// published Representation without copying its descriptors. Both establish
// agreement by comparing those canonical bytes.
using Representation = ttx_representation;

}  // namespace Ttx::Data::Form

constexpr auto ttx_representation_position::get_extent() const -> Count {
  return extent;
}

constexpr auto ttx_representation::get_extent() const -> Count {
  using namespace Ttx::Data::Encoding;
  const U8 depth = get_depth();
  return Struct::decode(get_blocks(), 0, depth).extent;
}

constexpr auto ttx_representation::get_alignment() const -> Count {
  using namespace Ttx::Data::Encoding;
  const U8 depth = get_depth();
  return Struct::decode(get_blocks(), 0, depth).alignment;
}

constexpr auto ttx_representation::compatible(
    const ttx_representation& other) const -> Bool {
  if (size != other.size) {
    return False;
  }

  if (data == other.data) {
    return True;
  }

  return Perimortem::Core::Data::compare(data, other.data, size);
}

template <typename Consumer>
constexpr auto ttx_representation::visit(Consumer consumer) const
    -> Ttx::Data::Status {
  using Ttx::Data::Status;
  using namespace Ttx::Data::Encoding;
  const U8 depth = get_depth();
  const auto bytes = get_blocks();
  // Keeping both widths explicit lets the compiler eliminate Position's
  // default width fallback without emitting a separate traversal for each.
  const bool narrow = get_pointer_size() == 4;
  const auto walk = [&](this auto&& self, Count body, Count offset) -> Status {
    const auto header = Struct::decode(bytes, body, depth);
    for (Count i = 0; i < header.count; ++i) {
      const auto entry = Element::decode(bytes, body + i + 1, depth);
      // The descriptor chooses one operation for the whole run. Keeping that
      // choice outside the loop lets primitive runs reuse their decoded facts
      // without testing for recursion at every position.
      if (entry.is_inline()) {
        for (Count repetition = 0; repetition < entry.count; ++repetition) {
          const auto status = self(
              entry.type, offset + entry.offset + repetition * entry.distance);
          if (status != Status::Success) {
            return status;
          }
        }
      } else {
        Position position(
            offset + entry.offset, entry.get_value(), entry.get_byte_order(),
            U32(entry.get_extent(narrow ? 4 : 8)));
        for (Count repetition = 0; repetition < entry.count; ++repetition) {
          const auto status = consumer(position);
          if (status != Status::Success) {
            return status;
          }

          position.offset += entry.distance;
        }
      }
    }

    return Status::Success;
  };
  return walk(0, 0);
}

template <typename Consumer>
constexpr auto ttx_representation::visit(
    Perimortem::Core::View::Vector<Count> coordinates,
    Consumer consumer) const -> Ttx::Data::Status {
  using Ttx::Data::Status;
  using namespace Ttx::Data::Encoding;
  const U8 depth = get_depth();
  Count selected = 0;

  const auto bytes = get_blocks();
  const bool narrow = get_pointer_size() == 4;
  const auto walk = [&](this auto&& self, Count body, Count offset) -> Status {
    const auto header = Struct::decode(bytes, body, depth);
    for (Count i = 0; i < header.count && selected < coordinates.get_size();
         ++i) {
      const auto entry = Element::decode(bytes, body + i + 1, depth);
      const Count first = offset + entry.offset;
      Count width;
      if (entry.is_inline()) {
        const auto child = Struct::decode(bytes, entry.type, depth);
        width = child.extent;
      } else {
        width = entry.get_extent(narrow ? 4 : 8);
      }

      const Count end = first + (entry.count - 1) * entry.distance + width;

      // The next requested coordinate selects an instance directly. Keeping
      // the descriptor here lets later requests in this same run reuse it.
      while (selected < coordinates.get_size() && coordinates[selected] < end) {
        const Count requested = coordinates[selected];
        if (requested < first) {
          return Status::Bounds;
        }

        const Count instance = (requested - first) / entry.distance;
        const Count start = first + instance * entry.distance;
        Status status;
        if (entry.is_inline()) {
          if (requested >= start + width) {
            return Status::Bounds;
          }

          const Count before = selected;
          status = self(entry.type, start);
          if (selected == before && status == Status::Success) {
            return Status::Bounds;
          }
        } else {
          if (requested != start) {
            return Status::Bounds;
          }

          const Position position(
              start, entry.get_value(), entry.get_byte_order(),
              U32(entry.get_extent(narrow ? 4 : 8)));
          status = consumer(position);
          ++selected;
        }

        if (status != Status::Success) {
          return status;
        }
      }
    }

    return Status::Success;
  };
  const auto status = walk(0, 0);
  if (status != Status::Success) {
    return status;
  }

  return selected == coordinates.get_size() ? Status::Success : Status::Bounds;
}

TTX_DATA_RECORD(
    ttx_representation,
    TTX_DATA_MEMBER(ttx_representation, data),
    TTX_DATA_MEMBER(ttx_representation, size));
