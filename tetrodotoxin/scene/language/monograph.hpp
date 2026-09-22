// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/scene/language/emission.hpp"
#include "tetrodotoxin/scene/language/lifecycle.hpp"
#include "tetrodotoxin/scene/language/signal.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Scene::Language {

// Monograph owns the relationships that make one Library Object a Scene. The
// child keeps the real Fields, Types, Functions, and executable Statements.
// Scene adds only Signals, lifecycle roles, and Emissions that Library cannot
// describe on its own. Keeping those identities connected gives editor,
// Archive, and Terminal consumers one graph instead of parallel Scene records.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Documentation& documentation,
      const Tetrodotoxin::Source::Abstract& language,
      Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Library::Language::Monograph& library,
      Tetrodotoxin::Library::Language::Types::Object& instance) -> Monograph&;

  auto retain_signal(Signal& signal, Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;
  auto retain_restored_signal(Signal& signal) -> Bool;
  auto retain_emission(Emission& emission) -> Bool;
  auto retain_lifecycle(
      Lifecycle role,
      Tetrodotoxin::Library::Language::Function& function,
      Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;
  auto retain_restored_lifecycle(
      Lifecycle role,
      Tetrodotoxin::Library::Language::Function& function) -> Bool;

  auto find_signal(Perimortem::Core::View::Bytes name) const
      -> Perimortem::Core::Option<const Signal&>;

  auto get_lifecycle(Lifecycle role) const -> Perimortem::Core::Option<
      const Tetrodotoxin::Library::Language::Function&>;

  constexpr auto get_signals() const { return signals.get_view(); }

  constexpr auto get_emissions() const { return emissions.get_view(); }

  constexpr auto is_finalized() const -> Bool {
    return stage == Stage::Finalized;
  }

  constexpr auto edit_library() -> Tetrodotoxin::Library::Language::Monograph& {
    return library;
  }

  constexpr auto get_library() const
      -> const Tetrodotoxin::Library::Language::Monograph& {
    return library;
  }

  constexpr auto edit_instance()
      -> Tetrodotoxin::Library::Language::Types::Object& {
    return instance;
  }

  constexpr auto get_instance() const
      -> const Tetrodotoxin::Library::Language::Types::Object& {
    return instance;
  }

  auto get_layer(const Tetrodotoxin::Source::Abstract& requested) const
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Language::Monograph&> override;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;
  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;

  auto link_restored() -> Bool override;
  auto finalize_restored() -> Bool override;

  TTX_NAME("Scene"_view);

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

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

 private:
  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Documentation& documentation,
      const Tetrodotoxin::Source::Abstract& language,
      Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Library::Language::Monograph& library,
      Tetrodotoxin::Library::Language::Types::Object& instance)
      : Tetrodotoxin::Language::Monograph(
            domain,
            language,
            documentation,
            context),
        library(library),
        instance(instance),
        signals(domain),
        emissions(domain) {}

  auto validate_lifecycle(Tetrodotoxin::Source::Lexical::Cursor& cursor) const -> Bool;
  auto validate_lifecycle_restored() const -> Bool;

  enum class Stage : U8 {
    Authored,
    Linked,
    Finalized,
  };

  Tetrodotoxin::Library::Language::Monograph& library;
  Tetrodotoxin::Library::Language::Types::Object& instance;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Source::Reference<Signal>> signals;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Source::Reference<Emission>>
      emissions;
  Perimortem::Core::Static::Vector<
      Perimortem::Core::Option<Tetrodotoxin::Library::Language::Function&>,
      5>
      lifecycle;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Scene::Language
