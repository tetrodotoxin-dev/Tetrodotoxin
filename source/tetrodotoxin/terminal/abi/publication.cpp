// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/publication.hpp"

#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"

auto Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  if (!definition.is_published()) {
    return False;
  }

  const Tetrodotoxin::Source::Abstract* selected = &definition.get_host();
  while (selected) {
    auto composite =
        selected->select<Tetrodotoxin::Library::Language::Types::Composite>();
    if (!composite || !composite->get_definition().is_published()) {
      return False;
    }
    if (composite->is<Tetrodotoxin::Library::Language::Types::Source>()) {
      return True;
    }
    selected = &composite->get_definition().get_host();
  }
  return False;
}
