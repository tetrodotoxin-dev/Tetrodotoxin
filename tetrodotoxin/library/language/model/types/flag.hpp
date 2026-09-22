// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"

namespace Tetrodotoxin::Library::Language::Model::Types {

class Flag : public Value {
 public:
  TTX_CONTRACT(Flag, Value);

  auto accepts_constant(const Tetrodotoxin::Source::Abstract&) const -> Bool override;

  // Logical consumers retain a Pack rather than a storage representation.
  // The exact Flag Type therefore owns interpretation of its first completed
  // value. A selected True means the condition is active and a selected False
  // means it is inactive. Absence reports a dynamic or incompatible value.
  virtual auto get_validity(const Model::Pack& value) const
      -> Perimortem::Core::Option<Bool> = 0;
};

}  // namespace Tetrodotoxin::Library::Language::Model::Types
