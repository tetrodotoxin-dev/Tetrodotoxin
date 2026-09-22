// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Negate owns one signed or real additive inverse. It retains the exact
// operand Expression and selects its Type during semantic linking. Folding
// projects a value without changing that authored input or linked Type.
class Negate : public Operation {
 public:
  TTX_CONTRACT(Negate, Operation);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& operand,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Negate&;
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& operand) -> Negate&;

  TTX_NAME("Negate"_view);

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Tetrodotoxin::Library::Language::Constant&>,
          Expression::Error> override;
  auto select_type(const Tetrodotoxin::Source::Abstract& context) const
      -> Perimortem::Core::Option<const Model::Type&> override;

 private:
  Negate(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& operand,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
