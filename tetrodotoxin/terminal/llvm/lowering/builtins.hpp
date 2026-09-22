// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/terminal/llvm/lowering/execution.hpp"
#include "tetrodotoxin/source/callable.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Lowering {

// Builtins maps generated Library Callables to target operations. Their
// signatures and folding remain semantic Library facts while native calling
// details stay on this side of the terminal boundary.
class Builtins {
 public:
  static auto lower(
      const Execution& execution,
      const Tetrodotoxin::Source::Callable& callable,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      Perimortem::Core::View::Vector<LLVMValueRef> inputs,
      Perimortem::Core::Option<
          const Tetrodotoxin::Library::Language::Model::Pack&> receiver_source)
      -> Perimortem::Core::Option<Bool>;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Lowering
