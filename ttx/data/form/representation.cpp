// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/data/form/representation.hpp"

using namespace Ttx::Data;
using namespace Ttx::Data::Form;

static auto header(const Representation& source, Count body)
    -> Encoding::Struct {
  return Encoding::Struct::decode(source.get_bytes(), body, source.get_depth());
}

static auto element(const Representation& source, Count block)
    -> Encoding::Element {
  return Encoding::Element::decode(
      source.get_bytes(), block, source.get_depth());
}

// Byte lookup can skip padding and whole instances. A coordinate inside a
// primitive asks for the next start, while an exact coordinate selects that
// primitive. Whole traversal uses visit instead of repeating this search.
static auto next(
    const Representation& source,
    Count body,
    Count offset,
    Count requested,
    Representation::Position& result) -> Bool {
  const auto form = header(source, body);
  for (Count i = 0; i < form.count; ++i) {
    const auto entry = element(source, body + i + 1);
    const Count first = offset + entry.offset;
    const Count local = requested > first ? requested - first : 0;
    Count instance = local / entry.distance;
    if (instance < entry.count) {
      const Count start = first + instance * entry.distance;
      if (entry.is_inline()) {
        if (next(source, entry.type, start, requested, result)) {
          return True;
        }

        ++instance;
        if (instance < entry.count) {
          return next(
              source, entry.type, first + instance * entry.distance, 0, result);
        }
      } else {
        instance += start < requested;
        if (instance < entry.count) {
          result = Representation::Position(
              first + instance * entry.distance, entry.get_value(),
              entry.get_byte_order());
          return True;
        }
      }
    }
  }

  return False;
}

auto ttx_representation_compatible(
    const Representation* source,
    const Representation* destination) -> U8 {
  return source && destination && source->compatible(*destination);
}

auto ttx_representation_next(
    const Representation* source,
    Count offset,
    Representation::Position* result) -> ttx_data_status {
  if (!source || !result) {
    return TTX_DATA_INVALID;
  }

  return next(*source, 0, 0, offset, *result) ? TTX_DATA_SUCCESS
                                              : TTX_DATA_BOUNDS;
}

auto ttx_representation::next(Count offset) const
    -> Perimortem::Utility::Result<Position, Status> {
  Position result;
  const auto status = ttx_representation_next(this, offset, &result);
  if (status != TTX_DATA_SUCCESS) {
    return static_cast<Status>(status);
  }

  return result;
}

auto ttx_representation_visit(
    const Representation* source,
    ttx_representation_visitor visitor) -> ttx_data_status {
  if (!source || !visitor.visit) {
    return TTX_DATA_INVALID;
  }

  return static_cast<ttx_data_status>(
      source->visit([&](Representation::Position position) {
        return static_cast<Status>(visitor.visit(visitor.source, position));
      }));
}

auto ttx_representation_visit_selected(
    const Representation* source,
    const Count* coordinates,
    Count count,
    ttx_representation_visitor visitor) -> ttx_data_status {
  if (!source || !visitor.visit || (count && !coordinates)) {
    return TTX_DATA_INVALID;
  }

  return static_cast<ttx_data_status>(source->visit(
      Perimortem::Core::View::Vector<Count>(coordinates, count),
      [&](Representation::Position position) {
        return static_cast<Status>(visitor.visit(visitor.source, position));
      }));
}

auto ttx_representation::compile(
    ttx_schema_reference schema,
    Perimortem::Memory::Allocator::Arena& arena)
    -> Perimortem::Utility::Result<const ttx_representation&, Status> {
  const ttx_representation_allocator allocator = {
    &arena, [](void* owner, Count bytes, Count) -> void* {
      return static_cast<Perimortem::Memory::Allocator::Arena*>(owner)
          ->allocate(bytes)
          .get_data();
    }};
  const ttx_representation* result = nullptr;
  const auto status = ttx_representation_compile(schema, allocator, &result);
  if (status != TTX_DATA_SUCCESS) {
    return static_cast<Status>(status);
  }

  return *result;
}
