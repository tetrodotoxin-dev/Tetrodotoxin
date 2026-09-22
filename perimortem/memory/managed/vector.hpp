// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/math.hpp"

#include "perimortem/memory/allocator/arena.hpp"

namespace Perimortem::Memory::Managed {

// A simple linear flat array of trivially constructable values.
template <typename value_type>
class Vector {
 public:
  static constexpr Count start_capacity = 8;
  static constexpr Count growth_factor = 2;

  Vector(const Vector&) = default;
  constexpr Vector(Allocator::Arena& arena) : arena(arena) { reset(); }

  constexpr operator Core::View::Vector<value_type>() const {
    return Core::View::Vector<value_type>(rented_block, size);
  }

  constexpr auto clear() -> void { size = 0; }

  constexpr auto reset() -> void {
    size = 0;
    capacity = start_capacity;
    rented_block = Core::Data::cast<value_type>(
        arena.allocate(sizeof(value_type) * start_capacity).get_data());
  }

  constexpr auto reset(Count reserve_capacity) -> void {
    if (reserve_capacity <= start_capacity) {
      reserve_capacity = start_capacity;
    }

    size = 0;
    capacity = reserve_capacity;
    rented_block = Core::Data::cast<value_type>(
        arena.allocate(sizeof(value_type) * reserve_capacity).get_data());
  }

  constexpr auto insert(const value_type& data) -> void {
    ensure_capacity(size + 1);

    // Construct using the copy constructor.
    new (rented_block + (size++), Core::Placement::Construct) value_type(data);
  }

  constexpr auto prepend(const value_type& data) -> void {
    ensure_capacity(size + 1);
    if (size != 0) {
      memmove(rented_block + 1, rented_block, sizeof(value_type) * size);
    }
    new (rented_block, Core::Placement::Construct) value_type(data);
    size++;
  }

  constexpr auto emplace(const value_type&& data) -> value_type& {
    ensure_capacity(size + 1);

    // Construct using the move constructor.
    return *new (rented_block + (size++), Core::Placement::Construct)
        value_type(data);
  }

  constexpr auto contains(const value_type& data) const -> Bool {
    return get_view().contains(data);
  }

  constexpr auto at(Count index) const -> value_type& {
    return rented_block[index];
  }

  constexpr auto operator[](Count index) -> value_type& { return at(index); }

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto is_empty() const -> Bool { return size == 0; }
  constexpr auto get_capacity() const -> Count { return capacity; };
  constexpr auto get_arena() const -> Allocator::Arena& { return arena; }
  constexpr auto get_view() const -> Core::View::Vector<value_type> {
    return Core::View::Vector<value_type>(rented_block, size);
  }

 private:
  // Ensures there is _at least_ enough room for the requested number of
  // objects.
  auto ensure_capacity(Count required_size) -> void {
    // Check if we can already fit required buffer.
    if (required_size <= get_capacity()) {
      return;
    }

    // Attempt to grow by a factor of 2.
    // If that doesn't work than grow to exact size.
    const auto new_capacity =
        Core::Math::max(get_capacity() * 2, required_size);

    // Fetch and transfer to new block.
    auto new_block = arena.reserve<value_type>(new_capacity).get_data();

    // Copy the raw bytes of the block
    if (rented_block) {
      memcpy((void*)new_block, rented_block, sizeof(value_type) * size);
    }

    // Update block and get the new capacity.
    rented_block = new_block;
    capacity = new_capacity;
  }

  Allocator::Arena& arena;
  value_type* rented_block;
  Count size;
  Count capacity;
};

}  // namespace Perimortem::Memory::Managed
