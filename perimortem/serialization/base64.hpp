// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::Serialization {

class Base64 {
 public:
  static auto decode(Core::View::Bytes source) -> Memory::Dynamic::Bytes;
  static auto decode(Memory::Allocator::Arena& arena, Core::View::Bytes source)
      -> Core::View::Bytes;

  static auto encode(Core::View::Bytes source) -> Memory::Dynamic::Bytes;
  static auto encode(Memory::Allocator::Arena& arena, Core::View::Bytes source)
      -> Core::View::Bytes;
};

}  // namespace Perimortem::Serialization
