// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/loop_control.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Language::Flow::LoopControl::create_authored(
    Allocator::Arena& domain,
    Kind kind,
    const Tetrodotoxin::Source::Abstract& target,
    Anchor anchor) -> LoopControl& {
  return domain.construct_from<LoopControl>(
      [&]() -> LoopControl { return LoopControl(kind, target, anchor); });
}
