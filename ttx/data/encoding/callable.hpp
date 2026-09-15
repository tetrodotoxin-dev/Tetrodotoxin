// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/encoding/element.hpp"

namespace Ttx::Data::Encoding {

// A callable block records the ABI and return carrier before its ordered
// argument runs. Its count is the expanded argument count, so adjacent equal
// arguments can share an Element block without erasing formal positions.
// Unlike storage members, those argument blocks have zero offset and distance:
// the ABI places them, rather than a shared memory frame described by Data.
struct Callable {
  Count count;
  Count abi;
  Element result;
  Bool returns_void;

  constexpr Callable(
      Count count,
      Count abi,
      Element result,
      Bool returns_void = False)
      : count(count), abi(abi), result(result), returns_void(returns_void) {}

  constexpr auto encode(U8 depth) const -> Block {
    const Count q = Count(depth) << 3;
    Block block;
    if (returns_void) {
      block.fill(0, q + 2);
    } else {
      block.insert(result.type, 0);
      block.insert(result.attributes & 7, q - 1);
    }

    block.insert(abi, 2 * q);
    block.insert(count, 3 * q);
    return block;
  }

  static constexpr auto decode(
      Perimortem::Core::View::Bytes bytes,
      Count index,
      U8 depth) -> Callable {
    const Count q = Count(depth) << 3;
    const Count first = (index * depth) << 5;
    const Count attributes = Block::extract(bytes, first + q - 1, 3);
    return Callable(
        Block::extract(bytes, first + 3 * q, q),
        Block::extract(bytes, first + 2 * q, q),
        Element(1, 0, 0, Block::extract(bytes, first, q - 1), attributes),
        (attributes & 6) == 6);
  }
};

}  // namespace Ttx::Data::Encoding
