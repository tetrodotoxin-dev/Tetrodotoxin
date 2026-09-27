// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/snapshots.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;

static constexpr Count maximum_snapshot_key = Path::max_size * 2 + 1;

static auto make_key(
    Static::Bytes<maximum_snapshot_key>& storage,
    View::Bytes package_root,
    View::Bytes logical_route) -> Option<View::Bytes> {
  Path root(package_root);
  Path route(logical_route);
  View::Bytes normalized_root = root.get_view();
  View::Bytes normalized_route = route.get_view();
  if (normalized_root.is_empty() || normalized_route.is_empty() ||
      route.is_rooted()) {
    return {};
  }

  Count required = normalized_root.get_size() + normalized_route.get_size() + 1;
  if (required > storage.get_size()) {
    return {};
  }

  Data::copy(
      storage.get_data(), normalized_root.get_data(),
      normalized_root.get_size());
  storage[normalized_root.get_size()] = '\0';
  Data::copy(
      storage.get_data() + normalized_root.get_size() + 1,
      normalized_route.get_data(), normalized_route.get_size());
  return View::Bytes(storage.get_data(), required);
}

auto Package::Snapshots::find(
    View::Bytes package_root,
    View::Bytes logical_route) -> Option<Entry&> {
  Static::Bytes<maximum_snapshot_key> storage;
  auto key = make_key(storage, package_root, logical_route);
  BAIL_IF(!key);

  auto selected = entries.find(*key);
  return selected ? Option<Entry&>(selected->value) : Option<Entry&>();
}

auto Package::Snapshots::create(
    View::Bytes package_root,
    View::Bytes logical_route) -> Option<Entry&> {
  Static::Bytes<maximum_snapshot_key> storage;
  auto key = make_key(storage, package_root, logical_route);
  BAIL_IF(!key);

  View::Bytes retained_key = arena.proxy(*key);
  auto entry = entries.insert(retained_key, Entry());
  return entry ? Option<Entry&>(entry->value) : Option<Entry&>();
}

auto Package::Snapshots::read(
    const File::Root& root,
    View::Bytes package_root,
    View::Bytes logical_route) -> Option<View::Bytes> {
  auto selected = find(package_root, logical_route);
  if (selected && selected->overlaid) {
    return selected->overlay_contents.get_view();
  }

  if (selected && selected->loaded) {
    auto fingerprint = root.fingerprint(logical_route);
    if (fingerprint && *fingerprint == selected->fingerprint) {
      return selected->contents.get_view();
    }
  }

  auto snapshot = root.read_snapshot(logical_route);
  if (!snapshot) {
    return {};
  }

  if (!selected) {
    selected = create(package_root, logical_route);
  }
  BAIL_IF(!selected);

  selected->contents = snapshot->take_contents();
  selected->fingerprint = snapshot->get_fingerprint();
  selected->loaded = True;
  return selected->contents.get_view();
}

auto Package::Snapshots::overlay(
    View::Bytes package_root,
    View::Bytes logical_route,
    View::Bytes contents) -> Bool {
  auto selected = find(package_root, logical_route);
  if (!selected) {
    selected = create(package_root, logical_route);
  }
  BAIL_IF(!selected);

  selected->overlay_contents = contents;
  selected->overlaid = True;
  return True;
}

auto Package::Snapshots::remove_overlay(
    View::Bytes package_root,
    View::Bytes logical_route) -> Bool {
  auto selected = find(package_root, logical_route);
  BAIL_IF(!selected || !selected->overlaid);

  selected->overlay_contents.clear();
  selected->overlaid = False;
  return True;
}
