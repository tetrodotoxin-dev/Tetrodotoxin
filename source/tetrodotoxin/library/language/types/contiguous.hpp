// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Contiguous is the shared Type contract for indexed value storage. The
// receiver remains one value of its exact concrete Type while this category
// exposes the exact element Type consumed by Slice. Access retains the writable
// subset used by Index.
class Contiguous : public Model::Type {
 public:
  TTX_CONTRACT(Contiguous, Model::Type);

  virtual constexpr auto get_element_type() const -> const Model::Type& = 0;

  constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> override {
    return get_element_type().get_declaration_anchor();
  }

  auto accepts_iteration(const Tetrodotoxin::Source::Layout& bindings) const
      -> Bool override;
};

}  // namespace Tetrodotoxin::Library::Language::Types
