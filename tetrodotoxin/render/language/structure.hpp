// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/render/language/declarations.hpp"
#include "tetrodotoxin/source/layout.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Render::Language {

// Structure is one authored Render contract context. Its nested declarations
// remain separate semantic identities, while its instance Layout contains only
// ordinary GPU values that participate in value flow.
class Structure : public Tetrodotoxin::Source::Type {
 public:
  TTX_CONTRACT(Structure, Tetrodotoxin::Source::Type);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Structure&;

  auto retain_addressable(
      Tetrodotoxin::Source::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_callable(
      Tetrodotoxin::Source::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_type(
      Tetrodotoxin::Source::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_instance(Tetrodotoxin::Source::Addressable& value) -> void;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  auto link_restored() -> Bool;

  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;

  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto visit_concepts(Tetrodotoxin::Source::Abstract::Visitor visitor) const
      -> void override;

  auto resolve_local_context(Perimortem::Core::View::Bytes name) const
      -> const Tetrodotoxin::Source::Abstract&;

  auto get_layout() const -> const Tetrodotoxin::Source::Layout& override;

  TTX_NAME(definition.get_name());
  TTX_DOCUMENTATION(definition.get_documentation());

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_addressables() const {
    return declarations.get_addressables();
  }

  constexpr auto get_callables() const { return declarations.get_callables(); }

  constexpr auto get_types() const { return declarations.get_types(); }

  constexpr auto get_instances() const { return instances.get_view(); }

 private:
  class InstanceLayout : public Tetrodotoxin::Source::Layout {
   public:
    constexpr InstanceLayout(const Structure& owner) : owner(owner) {}

    auto get_size() const -> Count override;
    auto get_abstract(Count index) const
        -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> override;
    auto get_name(Count index) const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> override;
    auto fits_entry(
        const Tetrodotoxin::Source::Layout& target,
        Count source_index,
        Count target_index) const -> Bool override;
    auto fits_at(const Tetrodotoxin::Source::Layout& target, Count target_offset) const
        -> Bool override;
    auto get_fitted_at(
        const Tetrodotoxin::Source::Layout& target,
        Count target_offset,
        Count target_index) const
        -> Perimortem::Utility::Result<
            const Tetrodotoxin::Source::Abstract&,
            Tetrodotoxin::Source::Layout::Errors> override;

   private:
    const Structure& owner;
  };

  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition)
      : definition(definition),
        declarations(domain),
        instances(domain),
        layout(*this) {}

  Tetrodotoxin::Language::Definition& definition;
  Declarations declarations;
  Perimortem::Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Addressable>>
      instances;
  InstanceLayout layout;
};

}  // namespace Tetrodotoxin::Render::Language
