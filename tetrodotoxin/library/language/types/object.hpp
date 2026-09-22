// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/types/structure.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Object is the managed nonnull reference specialization of Structure. It
// retains the mandatory authored Definition through Structure while Composite
// owns every member, lookup, Layout, and completion rule.
class Object : public Structure {
 protected:
  Object(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Bool provides_initialization = True);

 public:
  TTX_CONTRACT(Object, Structure);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Object&;

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Object&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Object&;

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto create_supplied(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Model::Pack& arguments,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto create_supplied_restored(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& arguments,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope)
      const -> Perimortem::Core::Option<Model::Pack&> override;
};

}  // namespace Tetrodotoxin::Library::Language::Types
