// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/value.hpp"

namespace Tetrodotoxin::Library::Language::Model::Types {

// Unsigned proves only the exact Library numeric category. Width remains on the
// selected Value, so category queries do not become an implicit conversion
// rule.
class Unsigned : public Value {
 public:
  TTX_CONTRACT(Unsigned, Value);

  auto accepts_constant(const Tetrodotoxin::Source::Abstract&) const -> Bool override;
};

}  // namespace Tetrodotoxin::Library::Language::Model::Types
