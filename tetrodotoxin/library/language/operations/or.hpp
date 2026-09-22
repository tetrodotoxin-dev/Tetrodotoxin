// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/operation.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Or owns left first logical disjunction over two matching Flag Types. Its
// authored edges stay intact while the selected Flag protocol decides whether
// the right edge is reachable from the completed left value.
class Or : public Operation {
 public:
  BINARY_OP_CONTRACT(Or);

 protected:
  auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Tetrodotoxin::Library::Language::Constant&>,
          Expression::Error> override;
  auto reaches_next_input(Count folded_input, const Constant& folded) const
      -> Bool override;
  auto select_type(const Tetrodotoxin::Source::Abstract& context) const
      -> Perimortem::Core::Option<const Model::Type&> override;

 private:
  Or(Perimortem::Memory::Allocator::Arena& domain,
     Model::Pack& left,
     Model::Pack& right,
     Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor);
};

}  // namespace Tetrodotoxin::Library::Language::Operations
