// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// LoopControl owns one authored `break` or `continue` statement. It retains the
// nearest enclosing loop identity so nested Blocks never reduce that semantic
// relationship to parser depth or a later lowering decision.
class LoopControl : public Tetrodotoxin::Source::Abstract {
 public:
  enum class Kind : U8 {
    Break,
    Continue,
  };

  TTX_CONTRACT(LoopControl, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Kind kind,
      const Tetrodotoxin::Source::Abstract& target,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> LoopControl&;

  LoopControl(const LoopControl&) = delete;
  LoopControl(LoopControl&&) = delete;
  auto operator=(const LoopControl&) -> LoopControl& = delete;
  auto operator=(LoopControl&&) -> LoopControl& = delete;

  TTX_NAME("LoopControl"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_kind() const -> Kind { return kind; }

  constexpr auto get_target() const -> const Tetrodotoxin::Source::Abstract& {
    return target.get();
  }

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

 private:
  constexpr LoopControl(
      Kind kind,
      const Tetrodotoxin::Source::Abstract& target,
      Tetrodotoxin::Source::Lexical::Anchor anchor)
      : kind(kind), target(target), anchor(anchor) {}

  Kind kind;
  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> target;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
