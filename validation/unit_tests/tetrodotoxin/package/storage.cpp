// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/storage.hpp"

#include "validation/unit_test.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/package/content.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Validation;

static_assert(
    static_cast<U8>(Package::Storage::Failure::Error::Unknown) == U8(-1));
static_assert(
    static_cast<U8>(Package::Storage::Failure::Error::InvalidRoute) == U8(0));
static_assert(
    static_cast<U8>(Package::Storage::Failure::Error::Unreadable) == U8(1));
static_assert(__is_trivially_destructible(Package::Storage::Failure));

static constexpr Count temporary_path_capacity = 160;
static constexpr Count cache_growth_count = 48;

static auto join_path(View::Bytes base, View::Bytes member) -> Dynamic::Bytes {
  Dynamic::Bytes path(base);
  path.append('/');
  path.concat(member);
  return path;
}

static auto native_path(Dynamic::Bytes& path) -> char* {
  path.append('\0');
  return Data::cast<char>(path.get_access().get_data());
}

static auto remove_member(View::Bytes base, View::Bytes member) -> void {
  Dynamic::Bytes path = join_path(base, member);
  File::remove(path);
}

static auto cleanup_tree(View::Bytes root) -> void {
  if (root.is_empty()) {
    return;
  }

  remove_member(root, "shared_a.ttx"_view);
  remove_member(root, "resource.bin"_view);
  remove_member(root, "empty.bin"_view);
  remove_member(root, "stable.bin"_view);
  remove_member(root, "snapshot.bin"_view);
  remove_member(root, "replacement.bin"_view);
  remove_member(root, "later.bin"_view);
  remove_member(root, "identity.bin"_view);
  remove_member(root, "hard_a.bin"_view);
  remove_member(root, "hard_b.bin"_view);
  remove_member(root, "escape.bin"_view);
  remove_member(root, "cwd_only.bin"_view);
  remove_member(root, "outside.bin"_view);
  remove_member(root, "sources/main.ttx"_view);
  remove_member(root, "sources/local.bin"_view);
  remove_member(root, "sources"_view);
  remove_member(root, "directory"_view);

  for (Count i = 0; i < cache_growth_count; i++) {
    Static::Bytes<32> member;
    S32 written = snprintf(
        Data::cast<char>(member.get_data()), member.get_size(),
        "cache_%02llu.bin", U64(i));
    if (written <= 0 || Count(written) >= member.get_size()) {
      continue;
    }

    remove_member(root, member.slice(0, Count(written)));
  }

  File::remove(root);
}

class TemporaryPackage {
 public:
  TemporaryPackage() {
    S32 root_written = snprintf(
        Data::cast<char>(root_path.get_data()), root_path.get_size(),
        "/tmp/tetrodotoxin_package_input_root_XXXXXX");
    if (root_written <= 0 || Count(root_written) >= root_path.get_size()) {
      return;
    }

    char* created_root = mkdtemp(Data::cast<char>(root_path.get_data()));
    if (created_root == nullptr) {
      return;
    }

    S32 outside_written = snprintf(
        Data::cast<char>(outside_path.get_data()), outside_path.get_size(),
        "/tmp/tetrodotoxin_package_input_outside_XXXXXX");
    if (outside_written <= 0 ||
        Count(outside_written) >= outside_path.get_size()) {
      cleanup_tree(get_root());
      return;
    }

    char* created_outside = mkdtemp(Data::cast<char>(outside_path.get_data()));
    if (created_outside == nullptr) {
      cleanup_tree(get_root());
      return;
    }

    valid = True;
  }

  TemporaryPackage(const TemporaryPackage&) = delete;
  auto operator=(const TemporaryPackage&) -> TemporaryPackage& = delete;

  ~TemporaryPackage() {
    if (!valid) {
      return;
    }

    cleanup_tree(get_root());
    cleanup_tree(get_moved_root());
    cleanup_tree(get_outside_root());
  }

  operator bool() const { return bool(valid); }

  auto get_root() const -> View::Bytes {
    return NullTerminated::to_view(
        Data::cast<const char>(root_path.get_data()));
  }

  auto get_moved_root() const -> View::Bytes {
    return NullTerminated::to_view(
        Data::cast<const char>(moved_path.get_data()));
  }

  auto get_outside_root() const -> View::Bytes {
    return NullTerminated::to_view(
        Data::cast<const char>(outside_path.get_data()));
  }

  auto write(View::Bytes member, View::Bytes contents) const -> Bool {
    Dynamic::Bytes path = join_path(get_root(), member);
    return File::write(contents, path);
  }

  auto write_outside(View::Bytes member, View::Bytes contents) const -> Bool {
    Dynamic::Bytes path = join_path(get_outside_root(), member);
    return File::write(contents, path);
  }

  auto remove(View::Bytes member) const -> Bool {
    Dynamic::Bytes path = join_path(get_root(), member);
    return File::remove(path);
  }

  auto create_directory(View::Bytes member) const -> Bool {
    Dynamic::Bytes path = join_path(get_root(), member);
    S32 created = mkdir(native_path(path), S_IRWXU);
    return created == 0;
  }

  auto create_escape_link(View::Bytes member, View::Bytes outside_member) const
      -> Bool {
    Dynamic::Bytes path = join_path(get_root(), member);
    Dynamic::Bytes target = join_path(get_outside_root(), outside_member);
    S32 linked = symlink(native_path(target), native_path(path));
    return linked == 0;
  }

  auto create_hard_link(View::Bytes existing_member, View::Bytes linked_member)
      const -> Bool {
    Dynamic::Bytes existing = join_path(get_root(), existing_member);
    Dynamic::Bytes linked = join_path(get_root(), linked_member);
    S32 created = link(native_path(existing), native_path(linked));
    return created == 0;
  }

  auto replace(View::Bytes replacement_member, View::Bytes target_member) const
      -> Bool {
    Dynamic::Bytes replacement = join_path(get_root(), replacement_member);
    Dynamic::Bytes target = join_path(get_root(), target_member);
    S32 moved = rename(native_path(replacement), native_path(target));
    return moved == 0;
  }

  auto rename_root() -> Bool {
    S32 written = snprintf(
        Data::cast<char>(moved_path.get_data()), moved_path.get_size(),
        "%s_moved", Data::cast<const char>(root_path.get_data()));
    if (written <= 0 || Count(written) >= moved_path.get_size()) {
      return False;
    }

    S32 moved = rename(
        Data::cast<const char>(root_path.get_data()),
        Data::cast<const char>(moved_path.get_data()));
    return moved == 0;
  }

  auto create_replacement_root() const -> Bool {
    S32 created = mkdir(Data::cast<const char>(root_path.get_data()), S_IRWXU);
    return created == 0;
  }

 private:
  Static::Bytes<temporary_path_capacity> root_path;
  Static::Bytes<temporary_path_capacity> moved_path;
  Static::Bytes<temporary_path_capacity> outside_path;
  Bool valid = False;
};

class WorkingDirectory {
 public:
  WorkingDirectory(View::Bytes location) {
    descriptor = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (descriptor < 0) {
      return;
    }

    Dynamic::Bytes native(location);
    S32 changed = chdir(native_path(native));
    entered = changed == 0;
  }

  WorkingDirectory(const WorkingDirectory&) = delete;
  auto operator=(const WorkingDirectory&) -> WorkingDirectory& = delete;

  ~WorkingDirectory() {
    if (descriptor < 0) {
      return;
    }

    fchdir(descriptor);
    close(descriptor);
  }

  operator bool() const { return bool(entered); }

  auto restore() -> Bool {
    if (descriptor < 0) {
      return False;
    }

    S32 restored = fchdir(descriptor);
    S32 closed = close(descriptor);
    descriptor = -1;
    entered = False;
    return restored == 0 && closed == 0;
  }

 private:
  S32 descriptor = -1;
  Bool entered = False;
};

struct TestConsumer {
  View::Bytes contents;
};

static auto select_content(
    const Result<Package::Content&, Package::Storage::Failure>& result)
    -> Package::Content* {
  return result.visit(
      [](Package::Content& content) { return &content; },
      [](const Package::Storage::Failure&) -> Package::Content* {
        return nullptr;
      });
}

static auto select_failure(
    const Result<Package::Content&, Package::Storage::Failure>& result)
    -> Option<const Package::Storage::Failure&> {
  return result.visit(
      [](Package::Content&) {
        return Option<const Package::Storage::Failure&>();
      },
      [](const Package::Storage::Failure& failure) {
        return Option<const Package::Storage::Failure&>(failure);
      });
}

static auto rejects_read(
    Package::Storage& storage,
    View::Bytes route,
    Package::Storage::Failure::Error expected_error,
    View::Bytes expected_path) -> Bool {
  auto result = storage.read(route);
  return result.visit(
      [](Package::Content&) { return False; },
      [&](const Package::Storage::Failure& failure) {
        if (failure.get_error() != expected_error) {
          return False;
        }

        auto path = failure.get_path();
        return path.visit(
            [&]() { return expected_path.is_empty() ? True : False; },
            [&](const Path& selected) {
              return !expected_path.is_empty() &&
                             selected.get_view() == expected_path
                         ? True
                         : False;
            });
      });
}

static Harness PackageStorage = {
  .name = "Tetrodotoxin::Package::Storage"_view,
};

PERIMORTEM_UNIT_TEST(PackageStorage, source_and_resources) {
  static constexpr View::Bytes root =
      "validation/data/ttx/package_resources"_view;
  static constexpr View::Bytes expected_header =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/"_view;
  Allocator::Arena arena;
  Dynamic::Bytes caller_route("resources/cache/../table.bin"_view);
  auto storage = Package::Storage::open(arena, root);
  ASSERT(storage);

  auto source_read = storage->read("./shared_a.ttx"_view);
  auto first_resource_read = storage->read(caller_route);
  Package::Content* source = select_content(source_read);
  Package::Content* first_resource = select_content(first_resource_read);
  ASSERT(source != nullptr);
  ASSERT(first_resource != nullptr);

  caller_route.set('x');
  auto second_resource_read = storage->read("resources/./table.bin"_view);
  auto empty_read = storage->read("resources/empty.bin"_view);
  Package::Content* second_resource = select_content(second_resource_read);
  Package::Content* empty = select_content(empty_read);
  ASSERT(second_resource != nullptr);
  ASSERT(empty != nullptr);

  EXPECT_TEXT(source->get_diagnostic_path(), "shared_a.ttx"_view);
  EXPECT_NOT(source->get_contents().is_empty());
  EXPECT_TEXT(
      first_resource->get_diagnostic_path(), "resources/table.bin"_view);
  EXPECT(
      first_resource->get_contents().slice(0, expected_header.get_size()) ==
      expected_header);
  EXPECT(
      first_resource->get_contents().get_data() ==
      second_resource->get_contents().get_data());
  EXPECT(
      first_resource->get_diagnostic_path().get_data() ==
      second_resource->get_diagnostic_path().get_data());
  EXPECT(first_resource == second_resource);
  EXPECT_TEXT(empty->get_diagnostic_path(), "resources/empty.bin"_view);
  EXPECT(empty->get_contents().is_empty());

  TestConsumer first_consumer{first_resource->get_contents()};
  TestConsumer second_consumer{second_resource->get_contents()};
  EXPECT(&first_consumer != &second_consumer);
  EXPECT(
      first_consumer.contents.get_data() ==
      second_consumer.contents.get_data());
}

PERIMORTEM_UNIT_TEST(PackageStorage, content_stability) {
  auto frozen = File::read(
      "validation/data/ttx/package_resources/resources/table.bin"_view);
  ASSERT(frozen);

  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.write("stable.bin"_view, *frozen));

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);

  auto original_read = storage->read("stable.bin"_view);
  Package::Content* original = select_content(original_read);
  ASSERT(original != nullptr);
  const U8* original_identity = original->get_contents().get_data();

  ASSERT(temporary.write("stable.bin"_view, "mutated"_view));
  auto mutation_read = storage->read("./stable.bin"_view);
  Package::Content* after_mutation = select_content(mutation_read);
  ASSERT(after_mutation != nullptr);
  EXPECT_TEXT(after_mutation->get_contents(), frozen->get_view());
  EXPECT(after_mutation->get_contents().get_data() == original_identity);

  ASSERT(temporary.write("replacement.bin"_view, "replacement"_view));
  ASSERT(temporary.replace("replacement.bin"_view, "stable.bin"_view));
  auto replacement_read = storage->read("stable.bin"_view);
  Package::Content* after_replacement = select_content(replacement_read);
  ASSERT(after_replacement != nullptr);
  EXPECT_TEXT(after_replacement->get_contents(), frozen->get_view());
  EXPECT(after_replacement->get_contents().get_data() == original_identity);

  ASSERT(temporary.remove("stable.bin"_view));
  auto removal_read = storage->read("stable.bin"_view);
  Package::Content* after_removal = select_content(removal_read);
  ASSERT(after_removal != nullptr);
  EXPECT_TEXT(after_removal->get_contents(), frozen->get_view());
  EXPECT(after_removal->get_contents().get_data() == original_identity);
}

PERIMORTEM_UNIT_TEST(PackageStorage, retry_after_failure) {
  TemporaryPackage temporary;
  ASSERT(temporary);

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);
  EXPECT(rejects_read(
      *storage, "cache/../later.bin"_view,
      Package::Storage::Failure::Error::Unreadable, "later.bin"_view));

  ASSERT(temporary.write("later.bin"_view, "available"_view));
  auto available_read = storage->read("cache/../later.bin"_view);
  Package::Content* available = select_content(available_read);
  ASSERT(available != nullptr);
  EXPECT_TEXT(available->get_diagnostic_path(), "later.bin"_view);
  EXPECT_TEXT(available->get_contents(), "available"_view);
}

PERIMORTEM_UNIT_TEST(PackageStorage, cache_growth) {
  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.write("stable.bin"_view, "stable"_view));

  Allocator::Arena arena;
  auto opened = Package::Storage::open(arena, temporary.get_root());
  ASSERT(opened);

  auto stable_read = opened->read("stable.bin"_view);
  Package::Content* stable_content = select_content(stable_read);
  ASSERT(stable_content != nullptr);
  View::Bytes stable_path = stable_content->get_diagnostic_path();
  View::Bytes stable_contents = stable_content->get_contents();

  for (Count i = 0; i < cache_growth_count; i++) {
    Static::Bytes<32> member;
    S32 written = snprintf(
        Data::cast<char>(member.get_data()), member.get_size(),
        "cache_%02llu.bin", U64(i));
    ASSERT(written > 0 && Count(written) < member.get_size());

    View::Bytes route = member.slice(0, Count(written));
    ASSERT(temporary.write(route, route));
    auto cache_read = opened->read(route);
    Package::Content* cached = select_content(cache_read);
    ASSERT(cached != nullptr);
    EXPECT_TEXT(cached->get_diagnostic_path(), route);
    EXPECT_TEXT(cached->get_contents(), route);
  }

  EXPECT_TEXT(stable_path, "stable.bin"_view);
  EXPECT_TEXT(stable_contents, "stable"_view);

  Package::Storage moved(static_cast<Package::Storage&&>(*opened));
  auto repeated_read = moved.read("./stable.bin"_view);
  Package::Content* repeated = select_content(repeated_read);
  ASSERT(repeated != nullptr);
  EXPECT(repeated == stable_content);
  EXPECT(repeated->get_diagnostic_path().get_data() == stable_path.get_data());
  EXPECT(repeated->get_contents().get_data() == stable_contents.get_data());
}

PERIMORTEM_UNIT_TEST(PackageStorage, opened_root_identity) {
  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.write("identity.bin"_view, "original root"_view));

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);
  ASSERT(temporary.rename_root());
  ASSERT(temporary.create_replacement_root());
  ASSERT(temporary.write("identity.bin"_view, "replacement root"_view));

  auto identity_read = storage->read("identity.bin"_view);
  Package::Content* identity = select_content(identity_read);
  ASSERT(identity != nullptr);
  EXPECT_TEXT(identity->get_contents(), "original root"_view);
}

PERIMORTEM_UNIT_TEST(PackageStorage, persistent_snapshots) {
  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.write("snapshot.bin"_view, "first"_view));

  Dynamic::Record<Package::Snapshots> snapshots;
  View::Bytes first_contents;
  {
    Allocator::Arena arena;
    auto storage =
        Package::Storage::open(arena, temporary.get_root(), snapshots);
    ASSERT(storage);
    auto read = storage->read("snapshot.bin"_view);
    Package::Content* content = select_content(read);
    ASSERT(content != nullptr);
    EXPECT_TEXT(content->get_contents(), "first"_view);
    first_contents = content->get_contents();
  }

  {
    Allocator::Arena arena;
    auto storage =
        Package::Storage::open(arena, temporary.get_root(), snapshots);
    ASSERT(storage);
    auto read = storage->read("snapshot.bin"_view);
    Package::Content* content = select_content(read);
    ASSERT(content != nullptr);
    EXPECT(content->get_contents().get_data() == first_contents.get_data());
  }

  ASSERT(snapshots->overlay(
      temporary.get_root(), "snapshot.bin"_view, "editor"_view));
  {
    Allocator::Arena arena;
    auto storage =
        Package::Storage::open(arena, temporary.get_root(), snapshots);
    ASSERT(storage);
    auto read = storage->read("snapshot.bin"_view);
    Package::Content* content = select_content(read);
    ASSERT(content != nullptr);
    EXPECT_TEXT(content->get_contents(), "editor"_view);
  }

  ASSERT(snapshots->remove_overlay(temporary.get_root(), "snapshot.bin"_view));
  ASSERT(temporary.write("snapshot.bin"_view, "second"_view));
  {
    Allocator::Arena arena;
    auto storage =
        Package::Storage::open(arena, temporary.get_root(), snapshots);
    ASSERT(storage);
    auto read = storage->read("snapshot.bin"_view);
    Package::Content* content = select_content(read);
    ASSERT(content != nullptr);
    EXPECT_TEXT(content->get_contents(), "second"_view);
  }
}

PERIMORTEM_UNIT_TEST(PackageStorage, releases_snapshot_values) {
  Count memory_before = Bibliotheca::allocated_memory();
  {
    Dynamic::Bytes contents;
    contents.append('A', 1 << 16);
    Dynamic::Record<Package::Snapshots> snapshots;
    ASSERT(snapshots->overlay(
        "/tmp/tetrodotoxin-snapshot-lifetime"_view, "source.ttx"_view,
        contents));
  }

  EXPECT_EQ(Bibliotheca::allocated_memory(), memory_before);
}

PERIMORTEM_UNIT_TEST(PackageStorage, route_rejections) {
  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.create_directory("directory"_view));
  ASSERT(temporary.create_directory("sources"_view));
  ASSERT(temporary.write("sources/main.ttx"_view, "source"_view));
  ASSERT(temporary.write("sources/local.bin"_view, "local"_view));
  ASSERT(temporary.write_outside("outside.bin"_view, "outside"_view));
  ASSERT(temporary.write_outside("cwd_only.bin"_view, "cwd"_view));
  ASSERT(temporary.create_escape_link("escape.bin"_view, "outside.bin"_view));

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);

  Dynamic::Bytes caller_failure_route("cache/../missing.bin"_view);
  auto owned_failure_read = storage->read(caller_failure_route);
  caller_failure_route.set('x');
  auto owned_failure = select_failure(owned_failure_read);
  ASSERT(owned_failure);
  EXPECT(
      owned_failure->get_error() ==
      Package::Storage::Failure::Error::Unreadable);
  auto owned_failure_path = owned_failure->get_path();
  EXPECT(owned_failure_path.visit(
      []() { return False; },
      [](const Path& selected) {
        return selected.get_view() == "missing.bin"_view;
      }));

  EXPECT(rejects_read(
      *storage, View::Bytes(), Package::Storage::Failure::Error::InvalidRoute,
      View::Bytes()));
  EXPECT(rejects_read(
      *storage, "."_view, Package::Storage::Failure::Error::InvalidRoute,
      View::Bytes()));
  EXPECT(rejects_read(
      *storage, "inside/.."_view,
      Package::Storage::Failure::Error::InvalidRoute, View::Bytes()));
  EXPECT(rejects_read(
      *storage, "/absolute.bin"_view,
      Package::Storage::Failure::Error::InvalidRoute, "/absolute.bin"_view));
  EXPECT(rejects_read(
      *storage, "\\rooted.bin"_view,
      Package::Storage::Failure::Error::InvalidRoute, "/rooted.bin"_view));
  EXPECT(rejects_read(
      *storage, "../outside.bin"_view,
      Package::Storage::Failure::Error::InvalidRoute, View::Bytes()));
  EXPECT(rejects_read(
      *storage, "inside/../../outside.bin"_view,
      Package::Storage::Failure::Error::InvalidRoute, View::Bytes()));
  EXPECT(rejects_read(
      *storage, "missing.bin"_view,
      Package::Storage::Failure::Error::Unreadable, "missing.bin"_view));
  EXPECT(rejects_read(
      *storage, "directory"_view, Package::Storage::Failure::Error::Unreadable,
      "directory"_view));
  EXPECT(rejects_read(
      *storage, "escape.bin"_view, Package::Storage::Failure::Error::Unreadable,
      "escape.bin"_view));

  Static::Bytes<9> nul_route = {{
    'n',
    'u',
    'l',
    'l',
    '\0',
    '.',
    'b',
    'i',
    'n',
  }};
  EXPECT(rejects_read(
      *storage, nul_route, Package::Storage::Failure::Error::InvalidRoute,
      View::Bytes()));

  auto source_read = storage->read("sources/main.ttx"_view);
  Package::Content* source = select_content(source_read);
  ASSERT(source != nullptr);
  EXPECT(rejects_read(
      *storage, "local.bin"_view, Package::Storage::Failure::Error::Unreadable,
      "local.bin"_view));

  WorkingDirectory outside(temporary.get_outside_root());
  ASSERT(outside);
  auto cwd_fallback = storage->read("cwd_only.bin"_view);
  Bool restored = outside.restore();
  auto cwd_failure = select_failure(cwd_fallback);
  ASSERT(cwd_failure);
  EXPECT(
      cwd_failure->get_error() == Package::Storage::Failure::Error::Unreadable);
  auto cwd_path = cwd_failure->get_path();
  EXPECT(cwd_path.visit(
      []() { return False; },
      [](const Path& selected) {
        return selected.get_view() == "cwd_only.bin"_view;
      }));
  EXPECT(restored);
}

PERIMORTEM_UNIT_TEST(PackageStorage, hard_link_routes) {
  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.write("hard_a.bin"_view, "same bytes"_view));
  ASSERT(temporary.create_hard_link("hard_a.bin"_view, "hard_b.bin"_view));

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);

  auto first_read = storage->read("hard_a.bin"_view);
  auto second_read = storage->read("hard_b.bin"_view);
  Package::Content* first = select_content(first_read);
  Package::Content* second = select_content(second_read);
  ASSERT(first != nullptr);
  ASSERT(second != nullptr);

  EXPECT_TEXT(first->get_contents(), second->get_contents());
  EXPECT(first->get_contents().get_data() != second->get_contents().get_data());
  EXPECT(
      first->get_diagnostic_path().get_data() !=
      second->get_diagnostic_path().get_data());

  auto repeated_read = storage->read("./hard_a.bin"_view);
  Package::Content* repeated = select_content(repeated_read);
  ASSERT(repeated != nullptr);
  EXPECT(first != second);
  EXPECT(first == repeated);
  EXPECT(
      first->get_contents().get_data() == repeated->get_contents().get_data());
}
