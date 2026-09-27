// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/types/signed.hpp"

#include "tetrodotoxin/library/language/constants/signed.hpp"

using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;

auto Model::Types::Signed::accepts_constant(const Abstract& value) const
    -> Bool {
  return value.visit<Constants::Signed>(
      [this](const Constants::Signed& selected) -> Bool {
        return &selected.get_type().resolve() == &resolve() ? True : False;
      },
      [](const Abstract&) -> Bool { return False; });
}
