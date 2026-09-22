// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/encoding/element.hpp"
#include "ttx/data/encoding/struct.hpp"
#include "ttx/data/form/representation.h"
#include "ttx/data/form/native.hpp"

namespace Ttx::Data::Form {

// C++ navigation uses the C record directly, so either language can borrow a
// published Representation without copying its descriptors. Both establish
// agreement by comparing those canonical bytes.
using Representation = ttx_representation;

}  // namespace Ttx::Data::Form

constexpr auto ttx_representation_position::get_extent() const -> Count {
  return Ttx::Data::Form::Schema::get_width(get_value());
}

constexpr auto ttx_representation::get_extent() const -> Count {
  using namespace Ttx::Data::Encoding;
  const U8 depth = get_depth();
  return Struct::decode(get_bytes(), 0, depth).extent;
}

constexpr auto ttx_representation::get_alignment() const -> Count {
  using namespace Ttx::Data::Encoding;
  const U8 depth = get_depth();
  return Struct::decode(get_bytes(), 0, depth).alignment;
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
  const auto walk = [&](this auto&& self, Count body, Count offset) -> Status {
    const auto header = Struct::decode(get_bytes(), body, depth);
    for (Count i = 0; i < header.count; ++i) {
      const auto entry = Element::decode(get_bytes(), body + i + 1, depth);
      for (Count repetition = 0; repetition < entry.count; ++repetition) {
        const Count start = offset + entry.offset + repetition * entry.distance;
        Status status;
        if (entry.is_inline()) {
          status = self(entry.type, start);
        } else {
          const Position position(
              start, entry.get_value(), entry.get_byte_order());
          status = consumer(position);
        }

        if (status != Status::Success) {
          return status;
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

  const auto walk = [&](this auto&& self, Count body, Count offset) -> Status {
    const auto header = Struct::decode(get_bytes(), body, depth);
    for (Count i = 0; i < header.count && selected < coordinates.get_size();
         ++i) {
      const auto entry = Element::decode(get_bytes(), body + i + 1, depth);
      const Count first = offset + entry.offset;
      Count width;
      if (entry.is_inline()) {
        const auto child = Struct::decode(get_bytes(), entry.type, depth);
        width = child.extent;
      } else {
        width = ttx_schema::get_width(entry.get_value());
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
              start, entry.get_value(), entry.get_byte_order());
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
