// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/resource.hpp"
#include "tetrodotoxin/package/snapshots.hpp"
#include "tetrodotoxin/source/lexical/associations.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/token.hpp"

namespace Tetrodotoxin::Environment {

// A Workspace is the lifetime of one connected semantic island. It retains the
// source transaction behind each Monograph so graph references can cross files
// and Dialects without copying identities into a global registry.
//
// Incomplete transactions keep their strongest available Tokens,
// Associations, diagnostics, and semantic edges for editor tooling. Linking
// and finalization decide when the entire island is complete enough for a
// Terminal producer. A later editor snapshot can release this Workspace as one
// lifetime and recompute dependents against the replacement source identities.
class Workspace : public Tetrodotoxin::Source::Abstract {
 public:
  class PackageSource {
   public:
    constexpr PackageSource(
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::View::Bytes logical_route,
        const Language::Monograph& monograph)
        : name(name), logical_route(logical_route), monograph(monograph) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_logical_route() const -> Perimortem::Core::View::Bytes {
      return logical_route;
    }
    constexpr auto get_monograph() const -> const Language::Monograph& {
      return monograph;
    }

   private:
    // The first route discovered from the Package root gives terminal products
    // one deterministic graph key. It is not an intrinsic Monograph name;
    // every authored Alias remains local to its importer.
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes logical_route;
    const Language::Monograph& monograph;
  };

  // Carries one Workspace owned editor projection. Semantic definitions retain
  // their authored Anchor and source text, while an acquired file uses an
  // empty Anchor to select the beginning of that physical input.
  class AuthoredLocation {
   public:
    constexpr AuthoredLocation(
        Perimortem::Core::View::Bytes package_root,
        Perimortem::Core::View::Bytes diagnostic_path,
        Perimortem::Core::View::Bytes source_text,
        Tetrodotoxin::Source::Lexical::Anchor anchor)
        : package_root(package_root),
          diagnostic_path(diagnostic_path),
          source_text(source_text),
          anchor(anchor) {}

    constexpr auto get_package_root() const -> Perimortem::Core::View::Bytes {
      return package_root;
    }

    constexpr auto get_diagnostic_path() const
        -> Perimortem::Core::View::Bytes {
      return diagnostic_path;
    }

    constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
      return source_text;
    }

    constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

   private:
    Perimortem::Core::View::Bytes package_root;
    Perimortem::Core::View::Bytes diagnostic_path;
    Perimortem::Core::View::Bytes source_text;
    Tetrodotoxin::Source::Lexical::Anchor anchor;
  };

  Workspace(
      Toolchain& toolchain,
      Perimortem::Core::Option<
          Perimortem::Memory::Dynamic::Record<Package::Snapshots>> snapshots =
          {});
  ~Workspace() override;

  // A direct source retains its transaction once its Dialect creates a
  // Monograph. The optional result still reports full semantic completion, so
  // build callers and editor callers can share one operation safely.
  auto interpret_source(
      Tetrodotoxin::Source::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes diagnostic_path,
      Perimortem::Core::View::Bytes contents)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // A Package begins with its restricted export source. Workspace follows
  // external Types relative to each importer and terminates at
  // exact Package facts already supplied by the terminal.
  auto import_package(
      Tetrodotoxin::Source::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes root_semantic_name,
      Perimortem::Core::View::Bytes root_logical_route)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // Archive restoration rebuilds a Package from facts that have already passed
  // archive validation, reacquires its recorded Type graph, and applies the
  // same completion order as authored sources.
  auto restore_package(
      const Package::Archive::Archive& archive,
      Perimortem::Core::View::Bytes root_semantic_name)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // Each retained Monograph keeps the authored index created in its source
  // transaction. Tooling can borrow the strongest identities interpretation
  // established even when later completion reports an error.
  auto get_associations(const Language::Monograph& monograph) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Lexical::Associations&>;

  auto get_associations(Perimortem::Core::View::Bytes diagnostic_path) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Lexical::Associations&>;

  auto get_associations(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Lexical::Associations&>;

  auto get_monograph(Perimortem::Core::View::Bytes diagnostic_path) const
      -> Perimortem::Core::Option<const Language::Monograph&>;

  auto get_monograph(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) const
      -> Perimortem::Core::Option<const Language::Monograph&>;

  auto get_completed_monograph(Perimortem::Core::View::Bytes diagnostic_path)
      const -> Perimortem::Core::Option<const Language::Monograph&>;

  auto get_completed_monograph(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) const
      -> Perimortem::Core::Option<const Language::Monograph&>;

  auto get_package_source_count(
      const Package::Language::Monograph& package) const -> Count;

  auto get_package_source(
      const Package::Language::Monograph& package,
      Count index) const -> Perimortem::Core::Option<PackageSource>;

  // Package imports terminate the local source walk. A terminal that can
  // acquire Packages may inspect these unresolved exact requests, load those
  // products, and rebuild the Workspace without duplicating source parsing.
  constexpr auto get_pending_package_imports() const
      -> Perimortem::Core::View::Vector<
          Tetrodotoxin::Source::Reference<Language::Import>> {
    return pending_package_imports;
  }

  // Keeping the original Token stream beside a retained source lets tooling
  // borrow the same lexical facts that built its semantic graph. That shared
  // view saves another tokenization pass and keeps source coordinates aligned.
  auto get_tokens(Perimortem::Core::View::Bytes diagnostic_path) const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Source::Lexical::Token>;

  auto get_tokens(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Source::Lexical::Token>;

  auto find_authored_location(const Tetrodotoxin::Source::Abstract& semantic) const
      -> Perimortem::Core::Option<AuthoredLocation>;

  // Tooling may project a selected locator back to the physical input that
  // satisfied it. This query uses retained Workspace acquisition facts and
  // does not add source paths to Import or Resource semantic identity.
  auto find_acquired_location(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route,
      Count offset,
      const Tetrodotoxin::Source::Abstract& semantic) const
      -> Perimortem::Core::Option<AuthoredLocation>;

  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override;
  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;
  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;
  auto visit_concepts(Tetrodotoxin::Source::Abstract::Visitor visitor) const
      -> void override;

 private:
  struct ImportedPackage {
    Perimortem::Core::View::Bytes identity;
    Perimortem::System::Version version;
    Package::Language::Monograph* monograph;
  };

  struct PackageMember {
    const Package::Language::Monograph* package;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes logical_route;
    const Language::Monograph* monograph;
  };

  // One retained source keeps its text, Tokens, authored index, semantic root,
  // and Arena together. Completion decides product eligibility without
  // discarding the evidence an editor can still use.
  struct RetainedSource {
    Perimortem::Core::View::Bytes package_root;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes logical_route;
    Perimortem::Core::View::Bytes diagnostic_path;
    Perimortem::Core::View::Bytes source_text;
    Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Allocator::Arena>
        transaction;
    Language::Monograph& monograph;
    Perimortem::Core::View::Vector<Tetrodotoxin::Source::Lexical::Token> tokens;
    const Tetrodotoxin::Source::Lexical::Associations& associations;
    Bool completed;
  };

  auto find_retained_source(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) const
      -> const RetainedSource*;

  // Workspace borrows one Toolchain for its full lifetime. Monographs can then
  // keep the exact installed Dialect identities without owning another
  // registry.
  Toolchain& toolchain;
  Perimortem::Core::Option<
      Perimortem::Memory::Dynamic::Record<Package::Snapshots>>
      snapshots;
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Dynamic::Vector<RetainedSource> retained_sources;
  Perimortem::Memory::Managed::Vector<PackageMember> package_members;
  Perimortem::Memory::Dynamic::Vector<
      Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Allocator::Arena>>
      restored_transactions;
  Perimortem::Memory::Dynamic::Map<
      Perimortem::Core::View::Bytes,
      Tetrodotoxin::Source::Reference<Language::Monograph>>
      retained_monographs;
  Perimortem::Memory::Managed::Vector<ImportedPackage> packages;
  Perimortem::Memory::Dynamic::Vector<Tetrodotoxin::Source::Reference<Language::Import>>
      pending_package_imports;
};

}  // namespace Tetrodotoxin::Environment
