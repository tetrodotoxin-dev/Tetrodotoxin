// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/terminal/abi/unit.hpp"
#include "tetrodotoxin/source/abstract.hpp"

namespace Tetrodotoxin::Terminal::Abi {

// Symbol owns one readable native spelling derived from semantic identity.
// Filesystem locations stay outside the path, so moving source keeps linkage
// stable.
class Symbol {
 public:
  enum class Kind : U8 {
    Path,
    FunctionStatic,
    FunctionSelf,
    Construction,
    Address,
    ReadOnly,
    ObjectDescriptor,
    Projection,
    GraphicsChildren,
  };

  Symbol(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Abstract& semantic,
      Kind kind,
      Unit unit = {});

  static auto validate(Perimortem::Core::View::Bytes value) -> Bool;

  constexpr auto get_view() const -> Perimortem::Core::View::Bytes {
    return value;
  }

 private:
  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Terminal::Abi
