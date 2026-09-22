// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/render/language/declarations.hpp"

namespace Tetrodotoxin::Render::Language {

// Monograph is the retained root of one Render contract source. Each concrete
// declaration enters the operator domain that can resolve it, while ordered
// views preserve the exact identities used by Shader and editor tooling.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Abstract& language,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Abstract& context) -> Monograph&;

  auto retain_addressable(
      Tetrodotoxin::Source::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_callable(
      Tetrodotoxin::Source::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_type(
      Tetrodotoxin::Source::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_import(
      const Tetrodotoxin::Language::Import::Description& description,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Associations&> associations = {})
      -> Bool override;

  auto compose(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;

  auto link_restored() -> Bool override;

  auto compose_restored() -> Bool override;

  auto finalize_restored() -> Bool override;

  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto visit_concepts(Tetrodotoxin::Source::Abstract::Visitor visitor) const
      -> void override;

  auto resolve_lexical_context(Perimortem::Core::View::Bytes name) const
      -> const Tetrodotoxin::Source::Abstract& override;

  constexpr auto get_addressables() const {
    return declarations.get_addressables();
  }

  constexpr auto get_callables() const { return declarations.get_callables(); }

  constexpr auto get_types() const { return declarations.get_types(); }

  constexpr auto is_finalized() const -> Bool { return finalized; }

  TTX_NAME("Pipeline"_view);

 private:
  Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Abstract& language,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Abstract& context)
      : Tetrodotoxin::Language::Monograph(
            arena,
            language,
            documentation,
            context),
        declarations(arena) {}

  Declarations declarations;
  Bool finalized = False;
};

}  // namespace Tetrodotoxin::Render::Language
