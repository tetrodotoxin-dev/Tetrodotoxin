// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/types/unsigned.hpp"

#include "tetrodotoxin/library/language/constants/unsigned.hpp"

using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;

auto Model::Types::Unsigned::accepts_constant(const Abstract& value) const
    -> Bool {
  return value.visit<Constants::Unsigned>(
      [this](const Constants::Unsigned& selected) -> Bool {
        return &selected.get_type().resolve() == &resolve() ? True : False;
      },
      [](const Abstract&) -> Bool { return False; });
}
