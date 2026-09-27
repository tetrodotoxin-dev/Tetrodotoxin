// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Render::Language {

// Declarations owns the three namespaces shared by Render roots and nested
// Structures. Every declaration is retained once in source order, while one
// visibility fact answers public queries without maintaining parallel vectors.
// This is Render policy rather than a generic language Scope because another
// language may define different namespaces, collisions, or publication rules.
class Declarations {
 public:
  constexpr Declarations(Perimortem::Memory::Allocator::Arena& domain)
      : authority(*this),
        published(domain),
        addressables(domain),
        callables(domain),
        types(domain) {}

  auto retain_addressable(
      Tetrodotoxin::Source::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;
  auto retain_callable(
      Tetrodotoxin::Source::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;
  auto retain_type(
      Tetrodotoxin::Source::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor, Tetrodotoxin::Source::Abstract& context)
      -> Bool;
  auto link_restored(Tetrodotoxin::Source::Abstract& context) -> Bool;

  constexpr auto get_authority() const -> const Tetrodotoxin::Source::Abstract& {
    return authority;
  }

  auto visit_concepts(Tetrodotoxin::Source::Abstract::Visitor visitor) const -> void;

  static auto resolve_lexical_context(
      const Tetrodotoxin::Source::Abstract& context,
      Perimortem::Core::View::Bytes name) -> const Tetrodotoxin::Source::Abstract&;

  auto resolve_addressable(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::Visibility visibility) const
      -> const Tetrodotoxin::Source::Abstract&;
  auto resolve_callable(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::Visibility visibility) const
      -> const Tetrodotoxin::Source::Abstract&;
  auto resolve_type(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::Visibility visibility) const
      -> const Tetrodotoxin::Source::Abstract&;

  constexpr auto get_addressables() const { return addressables.get_view(); }
  constexpr auto get_callables() const { return callables.get_view(); }
  constexpr auto get_types() const { return types.get_view(); }
  constexpr auto is_linked() const -> Bool { return linked; }

 private:
  class Authority : public Tetrodotoxin::Source::Abstract {
   public:
    constexpr explicit Authority(const Declarations& owner) : owner(owner) {}

    TTX_CONTRACT(Authority, Tetrodotoxin::Source::Abstract);
    TTX_NAME("static"_view);
    TTX_EMPTY_DOCUMENTATION();

    auto resolve_concept(Perimortem::Core::View::Bytes name) const
        -> const Tetrodotoxin::Source::Abstract& override;
    auto visit_concepts(Tetrodotoxin::Source::Abstract::Visitor visitor) const
        -> void override;

   private:
    const Declarations& owner;
  };

  auto retain(
      Perimortem::Memory::Managed::Vector<
          Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>>& declarations,
      Tetrodotoxin::Source::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;
  auto resolve(
      Perimortem::Core::View::Vector<
          Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>> declarations,
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::Visibility visibility) const
      -> const Tetrodotoxin::Source::Abstract&;

  Authority authority;
  Perimortem::Memory::Managed::Map<const Tetrodotoxin::Source::Abstract*, Bool>
      published;
  Perimortem::Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>>
      addressables;
  Perimortem::Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>>
      callables;
  Perimortem::Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>>
      types;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Render::Language
