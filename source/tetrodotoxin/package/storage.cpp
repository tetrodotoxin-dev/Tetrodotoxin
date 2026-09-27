// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/storage.hpp"

#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;

auto Package::Storage::open(
    Allocator::Arena& arena,
    View::Bytes location,
    Dynamic::Record<Package::Snapshots> snapshots) -> Option<Storage> {
  // Holding the opened descriptor keeps every read on the same package root
  // even if the pathname is renamed or replaced after Storage construction.
  return File::Root::open(location).visit(
      []() { return Option<Storage>(); },
      [&](File::Root& root) {
        return Option<Storage>(
            Storage(arena, snapshots, arena.proxy(location), Data::take(root)));
      });
}

auto Package::Storage::read(View::Bytes logical_route)
    -> Result<Content&, Failure> {
  // Storage accepts logical children only. Letting an absolute or escaping
  // route reach File would reintroduce ambient filesystem selection above the
  // opened root capability.
  Path normalized(logical_route);
  if (normalized.get_view().is_empty()) {
    return Failure(Failure::Error::InvalidRoute);
  }

  if (normalized.is_rooted()) {
    return Failure(normalized, Failure::Error::InvalidRoute);
  }

  // The normalized spelling is the only stable cache identity. Raw spellings
  // such as `a/./b` and `a/b` must observe the same first successful bytes.
  View::Bytes diagnostic_path = normalized.get_view();
  auto cached = cache.find(diagnostic_path);
  if (cached) {
    return cached->value;
  }

  auto contents = snapshots->read(root, root_path, diagnostic_path);
  if (!contents) {
    return Failure(normalized, Failure::Error::Unreadable);
  }

  // Arena allocation cannot be reclaimed individually. Delay the retained Path
  // until success so repeated cache hits and retryable failures do not grow the
  // acquisition Arena.
  auto retained_path = Path::normalize(arena, logical_route);
  if (!retained_path) {
    return Failure(normalized, Failure::Error::InvalidRoute);
  }

  // Launder only a fully stable Content into the reference map. Publishing
  // earlier could leave the cache pointing at a stack Path or failed read.
  Content& retained = arena.construct<Content>(*retained_path, *contents);
  cache.launder(*retained_path, retained);
  return retained;
}
