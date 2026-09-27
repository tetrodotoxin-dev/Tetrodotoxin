// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/execution.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Lowering {

// Values translates completed immutable Library values into the carrier facts
// selected by this Terminal. Constant domains remain entirely owned by Library.
class Values {
 public:
  static auto lower(
      const Execution& execution,
      const Tetrodotoxin::Library::Language::Constant& value) -> Bool;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Lowering
