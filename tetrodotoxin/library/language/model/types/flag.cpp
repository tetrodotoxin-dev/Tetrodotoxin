// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/types/flag.hpp"

#include "tetrodotoxin/library/language/constants/flag.hpp"

using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;

auto Model::Types::Flag::accepts_constant(const Abstract& value) const -> Bool {
  return value.visit<Constants::Flag>(
      [this](const Constants::Flag& selected) -> Bool {
        return &selected.get_type().resolve() == &resolve() ? True : False;
      },
      [](const Abstract&) -> Bool { return False; });
}
