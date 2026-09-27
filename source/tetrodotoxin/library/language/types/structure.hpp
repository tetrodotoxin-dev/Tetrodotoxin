// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Structure is one authored inline value Composite. Definition remains the
// single source for its name, Documentation, Attributes, modifiers, and kind.
class Structure : public Composite {
 public:
  TTX_CONTRACT(Structure, Composite);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Structure&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Structure&;

  Structure(const Structure&) = delete;
  Structure(Structure&&) = delete;
  auto operator=(const Structure&) -> Structure& = delete;
  auto operator=(Structure&&) -> Structure& = delete;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto create_fitted(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  constexpr auto has_initialization_provider() const -> Bool {
    return provides_initialization;
  }

  // The closing brace fixes member identity and source order even though
  // individual Type edges settle later. Source interpretation calls this once
  // after the complete authored body has been retained.
  auto complete_body() -> void override;

 protected:
  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Bool provides_initialization = True)
      : Composite(domain, definition),
        provides_initialization(provides_initialization) {}

  constexpr auto owns_initialization() const -> Bool {
    return provides_initialization;
  }

 private:
  Bool provides_initialization;
  mutable Bool creating_default = False;
};

}  // namespace Tetrodotoxin::Library::Language::Types
