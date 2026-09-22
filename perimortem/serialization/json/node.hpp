// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

namespace Perimortem::Serialization::Json {

class Node {
 public:
  // Object membership is recursive, so a Member retains its materialized Node
  // by reference. Both the Member range and every referred Node share the
  // caller's arena lifetime.
  class Member {
   public:
    constexpr Member(Core::View::Bytes name, const Node& node)
        : name(name), node(node) {}

    const Core::View::Bytes name;
    const Node& node;
  };

  Node() : data{.ptr = nullptr, .size = 0, .state = 0} { set(); }
  Node(const Node& rhs) : data(rhs.data) {};
  Node(const Core::View::Bytes value)
      : data{.ptr = nullptr, .size = 0, .state = 0} {
    set(value);
  }

  Node(const Core::View::Vector<Node> value)
      : data{.ptr = nullptr, .size = 0, .state = 0} {
    set(value);
  }

  Node(const Core::View::Vector<Member> value)
      : data{.ptr = nullptr, .size = 0, .state = 0} {
    set(value);
  }

  Node(S64 value) : data{.ptr = nullptr, .size = 0, .state = 0} { set(value); }

  Node(R32 value) : data{.ptr = nullptr, .size = 0, .state = 0} {
    set(R64(value));
  }

  Node(R64 value) : data{.ptr = nullptr, .size = 0, .state = 0} { set(value); }

  Node(Bool value) : data{.ptr = nullptr, .size = 0, .state = 0} { set(value); }

  auto set(const Core::View::Bytes value) -> void;
  auto set(const Core::View::Vector<Node> value) -> void;
  auto set(const Core::View::Vector<Member> value) -> void;
  auto set(S64 value) -> void;
  auto set(R64 value) -> void;
  auto set(Bool value) -> void;
  auto set() -> void;

  auto at(U32 index) const -> const Node;
  auto at(const Core::View::Bytes name) const -> const Node;

  auto operator[](U32 index) const -> const Node;
  auto operator[](const Core::View::Bytes name) const -> const Node;

  auto contains(const Core::View::Bytes name) const -> Bool;

  auto get_flag() const -> Bool;
  auto get_number() const -> S64;
  auto get_real() const -> double;
  auto get_string() const -> const Core::View::Bytes;
  auto decode_string(Memory::Allocator::Arena& arena) const
      -> Core::View::Bytes;
  auto get_array() const -> const Core::View::Vector<Node>;
  auto get_object() const -> const Core::View::Vector<Member>;

  // Gets the size in elements of any range type (string, array, object)
  // Returns 0 for any base types (null, flag, number, real)
  auto get_size() const -> Count;

  auto is_null() const -> Bool;
  auto is_flag() const -> Bool;
  auto is_number() const -> Bool;
  auto is_real() const -> Bool;
  auto is_string() const -> Bool;
  auto is_array() const -> Bool;
  auto is_object() const -> Bool;

  auto parse(
      Memory::Allocator::Arena& arena,
      Core::View::Bytes source,
      Count position = 0) -> Count;
  auto format(Memory::Allocator::Arena& arena) const -> Core::View::Bytes;

 private:
  // Node deliberately packs its scalar payload, range size, and state into 16
  // bytes. Static::Union would require separate storage for its tag and since
  // C++ can't unpack the struct it will tack it on to the end with padding.
  struct {
    union {
      const void* ptr;
      S64 number;
      R64 real;
      Bool flag;
    };
    U32 size;
    U32 state;
  } data;
};

static_assert(sizeof(Node) == 16, "Size of Node is required to be 16 bytes.");

using Object = Core::View::Vector<Node::Member>;
using Array = Core::View::Vector<Node>;

}  // namespace Perimortem::Serialization::Json
