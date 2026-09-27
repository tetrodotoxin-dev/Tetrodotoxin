// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/terminal/graphics/products.hpp"
#include "tetrodotoxin/terminal/llvm/module/program.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Lowering {

// Graphics compiles hosted Scene Fields and Fixed elements into one compact
// child access function. The runtime asks by authored order and receives the
// current Object together with its configured runtime Type index, while the
// semantic Field inventory ends with this Terminal transaction.
class Graphics {
 public:
  Graphics() = delete;

  static auto lower(
      Module::Program& program,
      const Tetrodotoxin::Terminal::Graphics::Products& products) -> Bool;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Lowering
