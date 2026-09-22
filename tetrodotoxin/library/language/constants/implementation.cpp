// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/implementation.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Constants::Implementation::create_empty(
    Perimortem::Memory::Allocator::Arena& domain,
    const Types::Implementation& type) -> Implementation& {
  return Constant::create_synthetic<Implementation>(
      domain, [&](auto source) -> Implementation {
        return Implementation(type, source);
      });
}

auto Constants::Implementation::equals(const Constant& rhs) const -> Bool {
  return rhs.is<Constants::Implementation>() && has_same_type(rhs);
}
