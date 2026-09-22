// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/terminal/llvm/module/emission.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Module {

// CAbi projects semantic LLVM carriers into the register shapes selected by
// the native C calling convention. The semantic Structure remains unchanged
// on either side of this boundary, which keeps target representation out of
// the graph while generated C declarations retain an ordinary C interface.
class CAbi {
 public:
  static auto select_direct_type(Emission& program, LLVMTypeRef semantic)
      -> Perimortem::Core::Option<LLVMTypeRef>;

  static auto convert(
      Emission& body,
      LLVMTypeRef target,
      LLVMValueRef value,
      Perimortem::Core::View::Bytes name)
      -> Perimortem::Core::Option<LLVMValueRef>;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Module
