// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// LessEqual owns one ordered scalar comparison. It retains the selected
// operand Type while its public result remains canonical Bool. Folding leaves
// those authored input and Type identities intact.
class LessEqual : public Operation {
 public:
  BINARY_OP_CONTRACT(LessEqual);

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Tetrodotoxin::Library::Language::Constant&>,
          Expression::Error> override;
  auto select_type(const Tetrodotoxin::Source::Abstract& context) const
      -> Perimortem::Core::Option<const Model::Type&> override;

 private:
  LessEqual(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& left,
      Model::Pack& right,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
