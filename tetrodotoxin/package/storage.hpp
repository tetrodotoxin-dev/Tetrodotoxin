// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/package/content.hpp"
#include "tetrodotoxin/package/snapshots.hpp"

namespace Tetrodotoxin::Package {

// Opens the physical root for one Package and caches every successful source
// or resource read by its canonical logical route.
//
// The importing source owns its local Alias name and relative spelling.
// Storage resolves only the canonical route into a diagnostic path and bytes.
// Content remains valid for this opened acquisition transaction. A semantic
// consumer copies any retained view into its own graph domain before Storage
// closes.
//
// The cache belongs to this opened Package storage only. Storage never
// interprets content or derives semantic identity from a route.
class Storage {
 public:
  // A failed read keeps only the normalized Path that Storage could establish
  // and the caller decision supported by File Root. Error stays nested because
  // it has no identity or use outside this one result.
  class Failure {
   public:
    enum class Error : U8 {
      Unknown = U8(-1),
      InvalidRoute = 0,
      Unreadable,
    };

    constexpr Failure(Error error) : error(error) {}

    constexpr Failure(Perimortem::System::Path path, Error error)
        : path(path), error(error) {}

    constexpr auto get_path() const
        -> Perimortem::Core::Option<const Perimortem::System::Path&> {
      if (path.get_view().is_empty()) {
        return {};
      }

      return path;
    }

    constexpr auto get_error() const -> Error { return error; }

   private:
    Perimortem::System::Path path;
    Error error;
  };

  Storage(const Storage&) = delete;
  auto operator=(const Storage&) -> Storage& = delete;
  Storage(Storage&&) = default;
  auto operator=(Storage&&) -> Storage& = delete;

  static auto open(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes root,
      Perimortem::Memory::Dynamic::Record<Snapshots> snapshots = {})
      -> Perimortem::Core::Option<Storage>;

  auto read(Perimortem::Core::View::Bytes logical_route)
      -> Perimortem::Utility::Result<Content&, Failure>;

 private:
  Storage(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Memory::Dynamic::Record<Snapshots> snapshots,
      Perimortem::Core::View::Bytes root_path,
      Perimortem::System::File::Root&& root)
      : arena(arena),
        snapshots(snapshots),
        root_path(root_path),
        root(static_cast<Perimortem::System::File::Root&&>(root)),
        cache(arena) {}

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Memory::Dynamic::Record<Snapshots> snapshots;
  Perimortem::Core::View::Bytes root_path;
  Perimortem::System::File::Root root;
  Perimortem::Memory::Managed::Map<Perimortem::Core::View::Bytes, Content&>
      cache;
};

}  // namespace Tetrodotoxin::Package
