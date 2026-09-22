// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/source/abstract.hpp"

namespace Tetrodotoxin::Terminal::Abi::Representation {

// Name gives a native producer one readable spelling for a representation
// fact. The semantic path and its ABI role remain shared even when individual
// producers realize that fact differently.
class Name {
 public:
  enum class Kind : U8 {
    ImplementationType,
    OptionType,
    ResultType,
    StructureType,
    ObjectType,
    ObjectFinalizer,
    ObjectDescriptor,
  };

  Name(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Abstract& semantic,
      Kind kind);

  constexpr auto get_view() const -> Perimortem::Core::View::Bytes {
    return value;
  }

 private:
  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Terminal::Abi::Representation
