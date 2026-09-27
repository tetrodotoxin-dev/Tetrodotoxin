// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"
#include "tetrodotoxin/package/snapshots.hpp"

namespace Puffer {

// Dependencies selects exact complete Package products in graph order. It
// checks the writable Terminal repository before the optional Package
// repository. Native builds select source roots so constructor bodies remain
// available, while Package consumers can restore complete graph archives.
class Dependencies {
 public:
  constexpr Dependencies(
      Perimortem::Memory::Allocator::Arena& arena,
      Tetrodotoxin::Package::Repository::Repository& terminal_repository,
      Perimortem::Core::Option<Tetrodotoxin::Package::Repository::Repository&>
          package_repository,
      Bool compile_sources = False)
      : arena(arena),
        terminal_repository(terminal_repository),
        package_repository(package_repository),
        archives(arena),
        compile_sources(compile_sources) {}

  auto acquire(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version) -> Bool;

  auto discover(
      Tetrodotoxin::Environment::Toolchain& toolchain,
      Perimortem::Core::View::Bytes root,
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::Option<Perimortem::Memory::Dynamic::Record<
          Tetrodotoxin::Package::Snapshots>> snapshots = {}) -> Bool;

  auto restore(Tetrodotoxin::Environment::Workspace& workspace) const -> Bool;

  constexpr auto get_archives() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Package::Archive::Archive> {
    return archives.get_view();
  }

 private:
  struct Source {
    Perimortem::Core::View::Bytes identity;
    Perimortem::System::Version version;
    Perimortem::Core::View::Bytes root;
  };
  auto acquire_source(
      Tetrodotoxin::Environment::Toolchain&,
      Perimortem::Core::View::Bytes,
      Perimortem::System::Version) -> Bool;
  struct Coordinate {
    Perimortem::Core::View::Bytes identity;
    Perimortem::System::Version version;
  };

  auto contains(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version) const -> Bool;

  Perimortem::Memory::Allocator::Arena& arena;
  Tetrodotoxin::Package::Repository::Repository& terminal_repository;
  Perimortem::Core::Option<Tetrodotoxin::Package::Repository::Repository&>
      package_repository;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Package::Archive::Archive>
      archives;
  Perimortem::Memory::Dynamic::Vector<Coordinate> active;
  Perimortem::Memory::Dynamic::Vector<Source> sources;
  Bool compile_sources;
};

}  // namespace Puffer
