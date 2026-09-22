// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/projection.hpp"

#include "tetrodotoxin/terminal/abi/symbol.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

auto Terminal::Abi::Projection::create(
    Memory::Allocator::Arena& arena,
    const Shader::Language::Program& program,
    Core::View::Bytes module_symbol,
    const Unit& unit) -> Core::Option<Projection> {
  BAIL_IF(module_symbol.is_empty() || !Symbol::validate(module_symbol));
  Symbol projection(
      arena, program.get_instance(), Symbol::Kind::Projection, unit);
  BAIL_IF(!Symbol::validate(projection.get_view()));
  return Projection(
      program, arena.proxy(module_symbol), arena.proxy(projection.get_view()));
}
