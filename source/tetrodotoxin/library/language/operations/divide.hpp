// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Divide owns one binary scalar quotient. It retains the exact left and right
// Expression edges and selects their shared Type during semantic linking.
// Folding projects a quotient while every authored input and linked Type
// remain.
class Divide : public Operation {
 public:
  BINARY_OP_CONTRACT(Divide);

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Tetrodotoxin::Library::Language::Constant&>,
          Expression::Error> override;
  auto select_type(const Tetrodotoxin::Source::Abstract& context) const
      -> Perimortem::Core::Option<const Model::Type&> override;

 private:
  Divide(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& left,
      Model::Pack& right,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
