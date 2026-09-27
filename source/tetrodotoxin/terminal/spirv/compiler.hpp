// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/terminal/spirv/failure.hpp"
#include "tetrodotoxin/terminal/spirv/products.hpp"
#include "tetrodotoxin/terminal/spirv/request.hpp"

namespace Tetrodotoxin::Terminal::Spirv {

// Compiler owns one complete Shader to SPIR V transaction. Products leave only
// after the graph walk and module framing both succeed.
class Compiler {
 public:
  auto compile(
      Perimortem::Memory::Allocator::Arena& arena,
      const Request& request) const
      -> Perimortem::Utility::Result<Products, Failure>;
};

}  // namespace Tetrodotoxin::Terminal::Spirv
