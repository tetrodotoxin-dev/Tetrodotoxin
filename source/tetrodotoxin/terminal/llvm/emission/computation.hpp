// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/terminal/llvm/module/body.hpp"
#include "tetrodotoxin/source/layout.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/callable.hpp"
#include "tetrodotoxin/source/pack.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Emission {

// Computation owns arithmetic and comparison instructions after Library fixes
// exact operand Types.
class Computation {
 public:
  enum class Arithmetic : U8 {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
  };

  enum class Comparison : U8 {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
  };

  constexpr Computation(Module::Body& body) : body(body) {}
  Computation(const Computation&) = delete;
  Computation(Computation&&) = delete;
  auto operator=(const Computation&) -> Computation& = delete;
  auto operator=(Computation&&) -> Computation& = delete;

  constexpr auto get_program() const -> Module::Program& {
    return body.get_program();
  }

  auto arithmetic(
      Arithmetic operation,
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Source::Pack& result,
      const Tetrodotoxin::Source::Pack& left,
      const Tetrodotoxin::Source::Pack& right) const -> Bool;
  auto negate(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Source::Pack& result,
      const Tetrodotoxin::Source::Pack& operand) const -> Bool;
  auto convert(
      const Tetrodotoxin::Source::Type& source_carrier,
      const Tetrodotoxin::Source::Type& target_carrier,
      const Tetrodotoxin::Source::Pack& result,
      const Tetrodotoxin::Source::Pack& source) const -> Bool;
  auto compare(
      Comparison operation,
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Source::Pack& result,
      const Tetrodotoxin::Source::Pack& left,
      const Tetrodotoxin::Source::Pack& right) const -> Bool;
  auto compare_bytes(
      Comparison operation,
      const Tetrodotoxin::Source::Pack& result,
      const Tetrodotoxin::Source::Pack& left,
      const Tetrodotoxin::Source::Pack& right) const -> Bool;

 private:
  Module::Body& body;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Emission
