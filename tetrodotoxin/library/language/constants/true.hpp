// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/constants/flag.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// True is the positive Flag Constant refinement. The retained Flag Type still
// owns representation while this identity exposes the closed logical value.
class True : public Flag {
 public:
  TTX_CONTRACT(True, Flag);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Library::Language::Model::Types::Flag& type,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> True& {
    return Constant::create_authored<True>(
        domain, anchor,
        [&](auto source) -> True { return True(type, source); });
  }

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Library::Language::Model::Types::Flag& type)
      -> True& {
    return Constant::create_synthetic<True>(
        domain, [&](auto source) -> True { return True(type, source); });
  }

 private:
  constexpr True(
      const Tetrodotoxin::Library::Language::Model::Types::Flag& type,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Flag(type, ::True, anchor) {}
};

}  // namespace Tetrodotoxin::Library::Language::Constants
