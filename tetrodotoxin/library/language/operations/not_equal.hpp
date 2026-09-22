// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// NotEqual owns exact semantic value inequality. Scalar and complete Bytes
// domains keep their own payload contracts while this operation retains only
// operand edges, their selected Type identity, and the canonical Bool result.
class NotEqual : public Operation {
 public:
  BINARY_OP_CONTRACT(NotEqual);

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Tetrodotoxin::Library::Language::Constant&>,
          Expression::Error> override;
  auto select_type(const Tetrodotoxin::Source::Abstract& context) const
      -> Perimortem::Core::Option<const Model::Type&> override;

 private:
  NotEqual(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& left,
      Model::Pack& right,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
