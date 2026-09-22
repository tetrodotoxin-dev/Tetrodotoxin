// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/value.hpp"

namespace Tetrodotoxin::Library::Language::Model::Types {

// Signed proves only the exact Library numeric category. Operations still
// require matching resolved Type identity and never infer widening or storage.
class Signed : public Value {
 public:
  TTX_CONTRACT(Signed, Value);

  auto accepts_constant(const Tetrodotoxin::Source::Abstract&) const -> Bool override;
};

}  // namespace Tetrodotoxin::Library::Language::Model::Types
