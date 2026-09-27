// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/terminal/llvm/lowering/execution.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Lowering {

// Access lowers selection, invocation, construction, and propagation from the
// exact identities already chosen by Library. It never performs name lookup or
// repeats semantic fitting.
class Access {
 public:
  static auto lower(
      const Execution& execution,
      const Tetrodotoxin::Library::Language::Expression& expression) -> Bool;

  static auto lower_write_target(
      const Execution& execution,
      const Tetrodotoxin::Library::Language::Expression& expression) -> Bool;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Lowering
