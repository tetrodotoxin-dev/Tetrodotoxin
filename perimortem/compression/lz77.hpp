// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

namespace Perimortem::Compression {

// Contains all of the Lz77 state used for compression passes.
// Note that Lz77 uses it's own mini hash table implementation since it's in the
// hot path of deflate. Any version of `Dynamic::Map` provides worse throughput.
class Lz77 {
 public:
  static constexpr Count min_match = 3;
  static constexpr Count max_match = 258;

  class Match {
   public:
    Match() : length(0), distance(0) {}
    Match(Count length, Count distance) : length(length), distance(distance) {}

    auto get_length() const -> Count { return length; }
    auto get_distance() const -> Count { return distance; }

   private:
    Count length;
    Count distance;
  };

  Lz77();

  auto reset() -> void;
  auto find_match(Core::View::Bytes source, Count position, Count depth) const
      -> Match;
  auto insert(Core::View::Bytes source, Count position) -> void;
  // Specialized merged find + insert which inserts position into the hash chain
  // then searches for the best match which avoids reading the hash table twice.
  auto find_match_and_insert(
      Core::View::Bytes source,
      Count position,
      Count depth) -> Match;

 private:
  Memory::Dynamic::Vector<U32> hash_table;
  Memory::Dynamic::Vector<U32> chain_table;
};

}  // namespace Perimortem::Compression
