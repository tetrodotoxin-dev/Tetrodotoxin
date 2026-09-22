// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/constants/flag.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// False is the negative Flag Constant refinement. It shares Flag equality and
// fitting while category proof can still select the exact logical value.
class False : public Flag {
 public:
  TTX_CONTRACT(False, Flag);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Library::Language::Model::Types::Flag& type,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> False& {
    return Constant::create_authored<False>(
        domain, anchor,
        [&](auto source) -> False { return False(type, source); });
  }

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Library::Language::Model::Types::Flag& type)
      -> False& {
    return Constant::create_synthetic<False>(
        domain, [&](auto source) -> False { return False(type, source); });
  }

 private:
  constexpr False(
      const Tetrodotoxin::Library::Language::Model::Types::Flag& type,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Flag(type, ::False, anchor) {}
};

}  // namespace Tetrodotoxin::Library::Language::Constants
