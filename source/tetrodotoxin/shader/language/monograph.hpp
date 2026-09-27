// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/shader/language/bridge.hpp"
#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Shader::Language {

// Monograph owns the Shader relationships authored by one source and one real
// Library child containing its executable meaning. The child exists because
// Shader bodies directly author Library Functions, Fields, Types, expressions,
// and Flow. Render contracts remain neighboring Workspace identities.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Abstract& language,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Library::Language::Monograph& library) -> Monograph&;

  auto retain_program(Program& program) -> Bool;
  auto retain_bridge(Bridge& bridge) -> Bool;

  auto compose(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;
  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;
  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;

  auto compose_restored() -> Bool override;
  auto link_restored() -> Bool override;
  auto finalize_restored() -> Bool override;

  auto get_layer(const Tetrodotoxin::Source::Abstract& requested) const
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Language::Monograph&> override;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto visit_concepts(Tetrodotoxin::Source::Abstract::Visitor visitor) const
      -> void override;

  auto resolve_lexical_context(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto retain_import(
      const Tetrodotoxin::Language::Import::Description& description,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Associations&> associations = {})
      -> Bool override {
    return library.retain_import(description, associations);
  }

  constexpr auto get_imports() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Language::Import>> override {
    return library.get_imports();
  }

  constexpr auto get_programs() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<Program>> {
    return programs;
  }

  constexpr auto get_bridges() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<Bridge>> {
    return bridges;
  }

  TTX_NAME("Shader"_view);

  constexpr auto edit_library() -> Tetrodotoxin::Library::Language::Monograph& {
    return library;
  }

  constexpr auto get_library() const
      -> const Tetrodotoxin::Library::Language::Monograph& {
    return library;
  }

  constexpr auto is_finalized() const -> Bool { return finalized; }

 private:
  // Shader overlays Material and bridge names on its Library child's public
  // scope. A separate lookup subject preserves that precedence without a
  // static route leading back to the Monograph. It borrows the existing
  // collections instead of maintaining another registry of their members.
  class Authority : public Tetrodotoxin::Source::Abstract {
   public:
    constexpr explicit Authority(const Monograph& owner) : owner(owner) {}

    TTX_CONTRACT(Authority, Tetrodotoxin::Source::Abstract);
    TTX_NAME("static"_view);
    TTX_EMPTY_DOCUMENTATION();

    auto resolve_concept(Perimortem::Core::View::Bytes name) const
        -> const Tetrodotoxin::Source::Abstract& override;
    auto visit_concepts(Tetrodotoxin::Source::Abstract::Visitor visitor) const
        -> void override;

   private:
    const Monograph& owner;
  };

  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Abstract& language,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Library::Language::Monograph& library)
      : Tetrodotoxin::Language::Monograph(
            domain,
            language,
            documentation,
            context),
        static_scope(*this),
        library(library),
        programs(domain),
        bridges(domain) {}

  Authority static_scope;
  Tetrodotoxin::Library::Language::Monograph& library;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Source::Reference<Program>>
      programs;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Source::Reference<Bridge>> bridges;
  Bool linked = False;
  Bool finalized = False;
};

}  // namespace Tetrodotoxin::Shader::Language
