// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/terminal/abi/unit.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Terminal::Abi {

// Projection is the ABI request for one concrete Shader Instance. It retains
// the real Program while the Workspace is live and the two stable native
// symbols needed to emit its process lifetime record.
class Projection {
 public:
  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Shader::Language::Program& program,
      Perimortem::Core::View::Bytes module_symbol,
      const Unit& unit) -> Perimortem::Core::Option<Projection>;

  constexpr auto get_program() const
      -> const Tetrodotoxin::Shader::Language::Program& {
    return program.get();
  }

  constexpr auto get_module_symbol() const -> Perimortem::Core::View::Bytes {
    return module_symbol;
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

 private:
  constexpr Projection(
      const Tetrodotoxin::Shader::Language::Program& program,
      Perimortem::Core::View::Bytes module_symbol,
      Perimortem::Core::View::Bytes symbol)
      : program(program), module_symbol(module_symbol), symbol(symbol) {}

  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Shader::Language::Program>
      program;
  Perimortem::Core::View::Bytes module_symbol;
  Perimortem::Core::View::Bytes symbol;
};

}  // namespace Tetrodotoxin::Terminal::Abi
