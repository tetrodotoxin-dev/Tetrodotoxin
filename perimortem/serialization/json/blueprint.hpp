// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/union.hpp"

#include "perimortem/serialization/json/node.hpp"

namespace Perimortem::Serialization::Json {

// Describes a temporary JSON tree which materializes its values as managed
// Nodes in an Arena with a single owning lifetime.
//
// Stack provided scalars, objects, arrays, and existing Nodes are supported.
// The value Union keeps each payload paired with its type without exposing a
// parallel tag or inactive fields.
class Blueprint {
 private:
  using Value = Core::Static::Union<
      Core::View::Bytes,
      Core::View::Vector<Blueprint>,
      Node,
      S64,
      R64,
      Bool>;

 public:
  Blueprint() = default;
  Blueprint(Core::View::Bytes text) : value(text) {}
  // Bool has its own JSON representation. Other integers use S64.
  template <typename Integer>
    requires(__is_integral(Integer) && !__is_same(Integer, bool))
  Blueprint(Integer number) : value(S64(number)) {}
  Blueprint(R64 real) : value(real) {}
  Blueprint(R32 real) : Blueprint(R64(real)) {}
  Blueprint(Bool flag) : value(flag) {}
  Blueprint(const Node& node) : value(node) {}

  Blueprint(Core::View::Bytes member_name, Core::View::Bytes text)
      : name(member_name), value(text) {}
  template <typename Integer>
    requires(__is_integral(Integer) && !__is_same(Integer, bool))
  Blueprint(Core::View::Bytes member_name, Integer number)
      : name(member_name), value(S64(number)) {}
  Blueprint(Core::View::Bytes member_name, R64 real)
      : name(member_name), value(real) {}
  Blueprint(Core::View::Bytes member_name, R32 real)
      : Blueprint(member_name, R64(real)) {}
  Blueprint(Core::View::Bytes member_name, Bool flag)
      : name(member_name), value(flag) {}
  Blueprint(Core::View::Bytes member_name, const Node& node)
      : name(member_name), value(node) {}

  static auto empty_array(Core::View::Bytes member_name = {}) -> Blueprint {
    return Blueprint(member_name, nullptr, 0);
  }

  template <Count N>
  Blueprint(Core::View::Bytes member_name, const Blueprint (&children)[N])
      : name(member_name), value(Core::View::Vector<Blueprint>(children)) {}

  template <Count N>
  Blueprint(const Blueprint (&children)[N])
      : value(Core::View::Vector<Blueprint>(children)) {}

  constexpr auto get_name() const -> Core::View::Bytes { return name; }

  // Recursively materializes the temporary tree into the caller's arena.
  // Named children become object members and unnamed children become array
  // elements.
  auto construct(Memory::Allocator::Arena& arena) const -> Node;

  template <typename... Cases>
  auto visit(Cases... cases) const -> decltype(auto) {
    return value.visit(static_cast<Cases&&>(cases)...);
  }

 private:
  Blueprint(
      Core::View::Bytes member_name,
      const Blueprint* children,
      Count child_count)
      : name(member_name),
        value(Core::View::Vector<Blueprint>(children, child_count)) {}

  Core::View::Bytes name;
  Value value;
};

static_assert(sizeof(Blueprint) == 40);

}  // namespace Perimortem::Serialization::Json
