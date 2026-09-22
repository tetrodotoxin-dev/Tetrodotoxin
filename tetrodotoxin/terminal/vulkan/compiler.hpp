// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/terminal/spirv/compiler.hpp"
#include "tetrodotoxin/terminal/vulkan/products.hpp"

namespace Tetrodotoxin::Terminal::Vulkan {

// Compiler owns the Vulkan generation boundary over one completed Shader
// Program. Module generation produces its SPIR V words, while description
// generation derives the matching CPU pipeline facts from the same graph.
class Compiler {
 public:
  auto compile_module(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Terminal::Spirv::Request& request) const
      -> Perimortem::Utility::Result<
          Tetrodotoxin::Terminal::Spirv::Products,
          Tetrodotoxin::Terminal::Spirv::Failure>;

  auto describe(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Shader::Language::Program& program,
      Perimortem::Core::View::Bytes symbol) const
      -> Perimortem::Core::Option<Products>;
};

}  // namespace Tetrodotoxin::Terminal::Vulkan
