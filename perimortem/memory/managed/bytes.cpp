// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/core/math.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;

Managed::Bytes::Bytes(const Bytes& rhs, Count reserved_capacity)
    : arena(rhs.arena) {
  reset(Math::max(rhs.get_size(), reserved_capacity));
  proxy(rhs);
};

Managed::Bytes::Bytes(Bytes&& rhs) : arena(rhs.arena) {
  // Take ownership and invalidate the old
  size = rhs.size;
  capacity = rhs.capacity;
  source_block = rhs.source_block;
};

Managed::Bytes::Bytes(Allocator::Arena& arena) : arena(arena) {
  reset();
}

Managed::Bytes::Bytes(Allocator::Arena& arena, View::Bytes view)
    : arena(arena) {
  reset();
  proxy(view);
}

auto Managed::Bytes::reset(Count reserved_capacity) -> void {
  size = 0;
  capacity = reserved_capacity;
  source_block = arena.allocate(reserved_capacity).get_data();
}

auto Managed::Bytes::resize(Count new_size) -> void {
  ensure_capacity(new_size);
  size = new_size;
}

auto Managed::Bytes::append(U8 byte) -> void {
  ensure_capacity(size + 1);
  source_block[size++] = byte;
}

auto Managed::Bytes::append(U8 byte, Count amount) -> void {
  ensure_capacity(size + amount);
  Data::set(source_block + size, byte, amount);
  size += amount;
}

auto Managed::Bytes::concat(View::Bytes view) -> void {
  ensure_capacity(size + view.get_size());

  memcpy(source_block + size, view.get_data(), view.get_size());
  size += view.get_size();
}

auto Managed::Bytes::proxy(View::Bytes view) -> void {
  resize(view.get_size());
  memcpy(source_block, view.get_data(), view.get_size());
}

auto Managed::Bytes::convert(U8 source, U8 target) -> void {
  for (int i = 0; i < size; i++) {
    if (source_block[i] == source) {
      source_block[i] = target;
    }
  }
}

auto Managed::Bytes::ensure_capacity(Count required_bytes) -> void {
  // Check if we can already fit required buffer.
  if (required_bytes <= capacity) {
    return;
  }

  // Attempt to grow by a factor of 2.
  // If that doesn't work than grow to exact size.
  const auto new_capacity = Math::max(capacity * 2, required_bytes);

  // Fetch and transfer to new block.
  auto new_block = arena.allocate(new_capacity).get_data();

  // Update block and get the new capacity.
  memcpy(new_block, source_block, size);
  source_block = new_block;
  capacity = new_capacity;
}
