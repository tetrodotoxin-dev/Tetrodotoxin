// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/foreign.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Source is one Library Monograph's synthetic root Composite. It owns synthetic
// Definition policy, source grammar, Static publication, and exactly one
// Foreign context. Repeated Foreign blocks merge into that identity while
// Composite keeps the shared inventories, lookup, and lifecycle.
class Source : public Composite {
 private:
  Source(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition)
      : Composite(domain, definition),
        foreign(domain, *this),
        import_routes(domain),
        imports(domain) {}

 protected:
  auto retain_binding(
      Tetrodotoxin::Source::Abstract& binding,
      Tetrodotoxin::Language::Definition& definition,
      Category category,
      Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;

 public:
  TTX_CONTRACT(Source, Composite);

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Abstract& host,
      const Tetrodotoxin::Source::Lexical::Anchor& source_anchor) -> Source&;

  Source(const Source&) = delete;
  Source(Source&&) = delete;
  auto operator=(const Source&) -> Source& = delete;
  auto operator=(Source&&) -> Source& = delete;

  auto retain_import_route(Import import) -> Bool;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Source::Abstract& interpretation_context) -> Bool;

  auto link_restored(Tetrodotoxin::Source::Abstract& interpretation_context) -> Bool;

  auto finalize_restored() -> Bool override;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;

  constexpr auto get_foreign() -> Foreign& { return foreign; }

  constexpr auto get_foreign() const -> const Foreign& { return foreign; }

  auto bind_static(
      Tetrodotoxin::Source::Abstract& binding,
      Category category,
      Bool published = False) -> Bool;

  auto can_bind_static(const Tetrodotoxin::Source::Abstract& binding, Category category)
      const -> Bool;

  auto resolve_imports(Perimortem::Core::View::Bytes name) const
      -> const Tetrodotoxin::Source::Abstract&;

  constexpr auto resolve() const -> const Tetrodotoxin::Source::Abstract& override {
    return *this;
  }

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto resolve_lexical_context(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto resolve_public_context(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto resolve_local(
      Perimortem::Core::View::Bytes route,
      Tetrodotoxin::Language::Visibility visibility =
          Tetrodotoxin::Language::Visibility::Public) const
      -> const Tetrodotoxin::Source::Abstract&;

  constexpr auto get_imports() const { return import_routes.get_view(); }

 private:
  auto retain_import_context(const Tetrodotoxin::Source::Abstract& context) -> Bool;
  auto link_imports(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Source::Abstract& interpretation_context) -> Bool;
  auto link_types(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;
  auto link_fields(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;
  auto link_initializers(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;
  auto link_callable_signatures(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;
  auto link_callable_bodies(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;

  Foreign foreign;
  Perimortem::Memory::Managed::Vector<Import> import_routes;
  Perimortem::Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>>
      imports;
  Bool imports_linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Types
