// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/hash.hpp"

#include "perimortem/memory/allocator/arena.hpp"

namespace Perimortem::Memory::Managed {

// A simple linear string which supports historical views on old data
// as long as the associated Arena is still alive.
class Bytes {
 public:
  static constexpr Count start_capacity = 32;
  static constexpr Count growth_factor = 2;

  Bytes(const Bytes& rhs, Count reserved_capacity = start_capacity);
  Bytes(Bytes&& rhs);
  Bytes(Allocator::Arena& arena);
  Bytes(Allocator::Arena& arena, Core::View::Bytes view);

  constexpr operator Core::View::Bytes() const { return get_view(); }
  constexpr operator Core::Access::Bytes() { return get_access(); }

  auto reset(Count reserved_capacity = start_capacity) -> void;

  auto resize(Count new_size) -> void;
  auto ensure_capacity(Count required_bytes) -> void;

  auto append(U8 byte) -> void;
  auto append(U8 byte, Count amount) -> void;
  auto concat(Core::View::Bytes view) -> void;
  // Copies a Core::View::Bytes which may be in a different allocator
  // (dynamic or another arena) into the Arena used by this object.
  auto proxy(Core::View::Bytes view) -> void;

  auto convert(U8 source, U8 target) -> void;

  constexpr auto operator[](Count index) const -> U8 {
    if (index >= size) {
      return 0;
    }

    return source_block[index];
  }

  constexpr auto at(Count index) const -> U8 {
    if (index >= size) {
      return 0;
    }

    return source_block[index];
  }

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_capacity() const -> Count { return capacity; }
  constexpr auto get_view() const -> const Core::View::Bytes {
    return Core::View::Bytes(source_block, size);
  }

  constexpr auto get_data() const -> const U8* { return source_block; }
  constexpr auto get_access() -> Core::Access::Bytes {
    return Core::Access::Bytes(source_block, size);
  }

  constexpr auto get_arena() const -> Allocator::Arena& { return arena; }

  constexpr auto hash() const -> U64 {
    return Core::Hash(get_view()).get_value();
  }

 private:
  Allocator::Arena& arena;
  U8* source_block;
  Count size;
  Count capacity;
};

}  // namespace Perimortem::Memory::Managed
