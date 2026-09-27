// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/dependencies.hpp"

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "tetrodotoxin/package/archive/graph_import.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

auto Puffer::Dependencies::contains(
    Core::View::Bytes identity,
    System::Version version) const -> Bool {
  return archives.get_view().contains(
      [&](const Package::Archive::Archive& archive) {
        return archive.get_identity() == identity &&
               archive.get_version() == version;
      });
}

auto Puffer::Dependencies::acquire(
    Core::View::Bytes identity,
    System::Version version) -> Bool {
  if (contains(identity, version)) {
    return True;
  }

  for (const Package::Archive::Archive& archive : archives.get_view()) {
    BAIL_IF(archive.get_identity() == identity);
  }

  for (const Coordinate& coordinate : active.get_view()) {
    BAIL_IF(coordinate.identity == identity && coordinate.version == version);
  }

  Core::Option<const Package::Archive::Archive&> selected;
  terminal_repository.select_archive(identity, version)
      .visit(
          [&](const Package::Archive::Archive& archive) { selected = archive; },
          [](Package::Repository::Repository::Error) {});
  if (!selected && package_repository) {
    package_repository->select_archive(identity, version)
        .visit(
            [&](const Package::Archive::Archive& archive) {
              selected = archive;
            },
            [](Package::Repository::Repository::Error) {});
  }
  BAIL_IF(!selected);

  active.insert(Coordinate(identity, version));
  Bool complete = True;

  // Exact Package Import edges order the complete closure before a fresh
  // Workspace receives any restored graph.
  for (const Package::Archive::GraphImport& import : selected->get_imports()) {
    if (import.get_kind() == Language::Import::Kind::Package) {
      complete &= acquire(import.get_target(), import.get_version());
    }
  }

  active.remove(active.get_size() - 1);
  if (complete) {
    archives.insert(*selected);
  }

  return complete;
}

// Native production needs source bodies and constructor providers. Complete
// graph archives remain useful for package inspection but do not supply those
// executable definitions in this compiler revision.
auto Puffer::Dependencies::acquire_source(
    Environment::Toolchain& toolchain,
    Core::View::Bytes identity,
    System::Version version) -> Bool {
  for (const auto& source : sources.get_view()) {
    if (source.identity == identity) {
      return source.version == version;
    }
  }
  for (const auto& coordinate : active.get_view()) {
    if (coordinate.identity == identity) {
      return False;
    }
  }
  Core::Option<Core::View::Bytes> root;
  terminal_repository.select_source(identity, version)
      .visit(
          [&](Core::View::Bytes path) { root = path; },
          [](Package::Repository::Repository::Error) {});
  if (!root && package_repository) {
    package_repository->select_source(identity, version)
        .visit(
            [&](Core::View::Bytes path) { root = path; },
            [](Package::Repository::Repository::Error) {});
  }
  if (!root) {
    return False;
  }
  active.insert(Coordinate(identity, version));
  const Bool complete =
      discover(toolchain, *root, identity, "package.ttx"_view);
  active.remove(active.get_size() - 1);
  // The requesting Import belongs to a temporary inspection Workspace. Retain
  // its name before that Workspace releases the source transaction.
  if (complete) {
    sources.insert(Source(arena.proxy(identity), version, *root));
  }
  return complete;
}

auto Puffer::Dependencies::restore(Environment::Workspace& workspace) const
    -> Bool {
  for (const auto& source : sources.get_view()) {
    Tetrodotoxin::Source::Lexical::Errors errors;
    if (!workspace.import_package(
            errors, source.root, source.identity, "package.ttx"_view)) {
      return False;
    }
  }
  for (const Package::Archive::Archive& archive : archives.get_view()) {
    BAIL_IF(!workspace.restore_package(archive, archive.get_identity()));
  }
  return True;
}

auto Puffer::Dependencies::discover(
    Environment::Toolchain& toolchain,
    Core::View::Bytes root,
    Core::View::Bytes semantic_name,
    Core::View::Bytes route,
    Core::Option<Memory::Dynamic::Record<Package::Snapshots>> snapshots)
    -> Bool {
  constexpr Count maximum_passes = 256;
  for (Count pass = 0; pass < maximum_passes; pass++) {
    Environment::Workspace inspection(toolchain, snapshots);
    BAIL_IF(!restore(inspection));
    Tetrodotoxin::Source::Lexical::Errors errors;
    if (inspection.import_package(errors, root, semantic_name, route)) {
      return True;
    }

    Count retained = archives.get_size() + sources.get_size();
    for (const Tetrodotoxin::Source::Reference<Language::Import>& pending :
         inspection.get_pending_package_imports()) {
      Bool acquired =
          compile_sources
              ? acquire_source(
                    toolchain, pending.get().get_locator(),
                    pending.get().get_version())
              : acquire(
                    pending.get().get_locator(), pending.get().get_version());
      BAIL_IF(!acquired);
    }
    if (archives.get_size() + sources.get_size() == retained) {
      // Discovery is complete even when the final semantic barriers reject the
      // source. Its real Workspace still retains that strongest partial graph.
      return True;
    }
  }
  return False;
}
