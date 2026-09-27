// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/terminal/llvm/lowering/execution.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Lowering {

// Operations lowers Library computation after the semantic owners have fixed
// exact operand and result Types. LLVM chooses instructions but never repeats
// Library legality or constant evaluation.
class Operations {
 public:
  static auto lower(
      const Execution& execution,
      const Tetrodotoxin::Library::Language::Expression& expression) -> Bool;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Lowering
