// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/terminal/llvm/module/program.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Module {

// Literals creates immutable native byte views for one LLVM Module. Ordinary
// language literals remain private to their compilation unit, while a Package
// Resource imports the one symbol published by its Package product. Both paths
// preserve the identity and lifetime already chosen by the graph.
class Literals {
 public:
  Literals() = delete;

  static auto create_bytes_view(
      Program& program,
      LLVMTypeRef type,
      Perimortem::Core::View::Bytes value,
      Perimortem::Core::Option<const Tetrodotoxin::Language::Resource&>
          resource = {}) -> Perimortem::Core::Option<LLVMValueRef>;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Module
