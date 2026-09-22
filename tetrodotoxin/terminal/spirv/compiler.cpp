// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/spirv/compiler.hpp"

#include "tetrodotoxin/terminal/spirv/module/program.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;

auto Spirv::Compiler::compile(
    Memory::Allocator::Arena& arena,
    const Request& request) const -> Utility::Result<Products, Failure> {
  Spirv::Module::Program program(arena, request);
  return program.compile();
}
