// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/encoding/block.hpp"

namespace Ttx::Data::Encoding {

// A struct block precedes its member descriptors and preserves the containing
// object's extent and alignment. Compaction can therefore merge member runs
// without losing the padding or boundary that distinguishes this object from
// an otherwise identical sequence of primitives.
struct Struct {
  Count count;
  Count alignment;
  Count extent;

  constexpr Struct(Count count, Count alignment, Count extent)
      : count(count), alignment(alignment), extent(extent) {}

  constexpr auto encode(U8 depth) const -> Block {
    const Count q = Count(depth) << 3;
    Block block;
    block.insert(depth, 0);
    block.insert(count, 4);
    block.insert(alignment, 4 + q);
    block.insert(extent, 4 + 2 * q);
    return block;
  }

  static constexpr auto decode(
      Perimortem::Core::View::Bytes bytes,
      Count index,
      U8 depth) -> Struct {
    const Count q = Count(depth) << 3;
    const Count first = (index * depth) << 5;
    return Struct(
        Block::extract(bytes, first + 4, q),
        Block::extract(bytes, first + 4 + q, q),
        Block::extract(bytes, first + 4 + 2 * q, (Count(depth) << 4) - 4));
  }
};

}  // namespace Ttx::Data::Encoding
