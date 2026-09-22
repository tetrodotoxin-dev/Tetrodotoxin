// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/types/composite.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Namespace is a contextual Type that publishes only nested Types and Aliases.
// It has no value Layout, state, construction, or Callable surface.
class Namespace : public Composite {
 public:
  TTX_CONTRACT(Namespace, Composite);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Namespace&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Namespace&;

  auto complete_body() -> void override;

  auto create_default(Perimortem::Memory::Allocator::Arena&) const
      -> Perimortem::Core::Option<Model::Pack&> override;

 protected:
  auto retain_binding(
      Tetrodotoxin::Source::Abstract& binding,
      Tetrodotoxin::Language::Definition& definition,
      Category category,
      Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;

 private:
  Namespace(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition)
      : Composite(domain, definition) {}
};

}  // namespace Tetrodotoxin::Library::Language::Types
