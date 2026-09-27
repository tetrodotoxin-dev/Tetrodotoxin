// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/terminal/abi/projection.hpp"
#include "tetrodotoxin/terminal/llvm/module/program.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Lowering {

// Projections emits each immutable mapping from one Shader Instance Object to
// its compiled Program and Parameters subobject. The semantic identities remain
// request keys while only byte offsets, sizes, and symbols cross the Terminal
// boundary.
class Projections {
 public:
  Projections() = delete;

  static auto lower(
      Module::Program& program,
      Perimortem::Core::View::Vector<Tetrodotoxin::Terminal::Abi::Projection>
          projections) -> Bool;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Lowering
