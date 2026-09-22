// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/types/structure.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Interface is one authored semantic requirement. Its Fields and Callables
// describe the surface a concrete Object supplies, while its empty Layout keeps
// the requirement itself out of ordinary value flow.
class Interface : public Structure {
 public:
  TTX_CONTRACT(Interface, Structure);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Interface&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Interface&;

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto create_fitted(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto get_layout() const -> const Tetrodotoxin::Source::Layouts::Named& override;

  // Interface requirements do not enter value flow, but their ordered state
  // still defines the prefix materialized by every explicit implementation.
  // Native Terminals consume that exact declaration-owned projection without
  // turning the Interface itself into a runtime value.
  auto get_state_layout() const -> const Tetrodotoxin::Source::Layouts::Named&;

 private:
  Interface(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition)
      : Structure(domain, definition, False) {}

  auto retain_binding(
      Tetrodotoxin::Source::Abstract& binding,
      Tetrodotoxin::Language::Definition& definition,
      Category category,
      Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;
};

}  // namespace Tetrodotoxin::Library::Language::Types
