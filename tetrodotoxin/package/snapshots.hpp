// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/map.hpp"

#include "perimortem/system/file.hpp"

namespace Tetrodotoxin::Package {

// Snapshots owns reusable nonsemantic filesystem bytes across complete
// Workspace graph replacements. Package Storage remains responsible for route
// normalization and confinement, while this owner compares metadata from the
// exact opened member before reusing an immutable byte value.
class Snapshots {
 public:
  Snapshots() = default;

  auto read(
      const Perimortem::System::File::Root& root,
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route)
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

  // An editor overlay is authoritative until removed. Empty text remains a
  // real overlay and never falls through to the filesystem snapshot.
  auto overlay(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route,
      Perimortem::Core::View::Bytes contents) -> Bool;

  auto remove_overlay(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) -> Bool;

 private:
  struct Entry {
    constexpr Entry() = default;

    Perimortem::Memory::Dynamic::Bytes contents;
    Perimortem::Memory::Dynamic::Bytes overlay_contents;
    Perimortem::System::File::Fingerprint fingerprint;
    Bool loaded = False;
    Bool overlaid = False;
  };

  auto find(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route)
      -> Perimortem::Core::Option<Entry&>;

  auto create(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route)
      -> Perimortem::Core::Option<Entry&>;

  // The Arena gives lookup keys stable storage while the Dynamic map gives
  // each owned byte value its ordinary destructor when this cache is released.
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Dynamic::Map<Perimortem::Core::View::Bytes, Entry>
      entries;
};

}  // namespace Tetrodotoxin::Package
