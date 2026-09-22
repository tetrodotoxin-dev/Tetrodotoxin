// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/terminal/llvm/failure.hpp"
#include "tetrodotoxin/terminal/llvm/products.hpp"
#include "tetrodotoxin/terminal/llvm/request.hpp"

namespace Tetrodotoxin::Terminal::Llvm {

// Compiler owns the LLVM Library compilation entry. Every call is one complete
// transaction and publishes products only after verification succeeds.
class Compiler {
 public:
  constexpr Compiler() = default;

  auto link(
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> arguments)
      const -> Bool;

  auto compile(
      Perimortem::Memory::Allocator::Arena& arena,
      const Request& request) const
      -> Perimortem::Utility::Result<Products, Failure>;
};

}  // namespace Tetrodotoxin::Terminal::Llvm
