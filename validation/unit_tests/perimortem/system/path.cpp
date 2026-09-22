// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/path.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Validation;

static Harness SystemPath = {
  .name = "System::Path"_view,
};

PERIMORTEM_UNIT_TEST(SystemPath, normalize) {
  Path path("unit\\./folder//file.ttx"_view);

  EXPECT_NOT(path.get_view().is_empty());
  EXPECT_TEXT(path.get_view(), "unit/folder/file.ttx"_view);
  EXPECT_NOT(path.is_rooted());
}

PERIMORTEM_UNIT_TEST(SystemPath, normalize_in_arena) {
  Allocator::Arena arena;
  Dynamic::Bytes authored("unit\\./folder//file.ttx"_view);
  auto normalized = Path::normalize(arena, authored);
  auto rooted = Path::normalize(arena, "/usr//local/./bin"_view);
  ASSERT(normalized);
  ASSERT(rooted);

  // Mutating the source separates Arena ownership from a view that merely
  // happens to contain the expected bytes during the call.
  authored.set('x');

  EXPECT_TEXT(*normalized, "unit/folder/file.ttx"_view);
  EXPECT_TEXT(*rooted, "/usr/local/bin"_view);
}

PERIMORTEM_UNIT_TEST(SystemPath, normalize_invalid) {
  Allocator::Arena arena;
  Static::Bytes<3> embedded_nul = {{'a', '\0', 'b'}};
  Static::Bytes<Path::max_size + 1> oversized;
  for (Count i = 0; i < oversized.get_size(); i++) {
    oversized[i] = 'a';
  }

  // The oversized case uses one segment so failure proves the capacity bound
  // rather than an unrelated parent or separator rule.
  EXPECT_NOT(Path::normalize(arena, View::Bytes()));
  EXPECT_NOT(Path::normalize(arena, "."_view));
  EXPECT_NOT(Path::normalize(arena, "../file.ttx"_view));
  EXPECT_NOT(Path::normalize(arena, embedded_nul));
  EXPECT_NOT(Path::normalize(arena, oversized));
}

PERIMORTEM_UNIT_TEST(SystemPath, relative) {
  Path path("unit/source/main.ttx"_view, "../shared/a.ttx"_view);

  EXPECT_NOT(path.get_view().is_empty());
  EXPECT_TEXT(path.get_view(), "unit/shared/a.ttx"_view);
}

PERIMORTEM_UNIT_TEST(SystemPath, rooted) {
  Path path("/usr/local/bin"_view);

  EXPECT_NOT(path.get_view().is_empty());
  EXPECT_TEXT(path.get_view(), "/usr/local/bin"_view);
  EXPECT(path.is_rooted());
}

PERIMORTEM_UNIT_TEST(SystemPath, escape) {
  Path path("../file.ttx"_view);
  Path relative_path("unit/main.ttx"_view, "../../file.ttx"_view);

  EXPECT(path.get_view().is_empty());
  EXPECT(relative_path.get_view().is_empty());
}

PERIMORTEM_UNIT_TEST(SystemPath, root_parent) {
  Path path("/usr/.."_view);

  EXPECT_NOT(path.get_view().is_empty());
  EXPECT_TEXT(path.get_view(), "/"_view);
  EXPECT(path.is_rooted());
}

PERIMORTEM_UNIT_TEST(SystemPath, file) {
  Path path("unit/source/main.ttx"_view);

  EXPECT_TEXT(path.get_file(), "main.ttx"_view);
}

PERIMORTEM_UNIT_TEST(SystemPath, directory) {
  Path path("unit/source/main.ttx"_view);
  Path root_child("/main.ttx"_view);

  EXPECT_TEXT(path.get_directory(), "unit/source"_view);
  EXPECT_TEXT(root_child.get_directory(), "/"_view);
}

PERIMORTEM_UNIT_TEST(SystemPath, extension) {
  Path path("unit/source/main.ttx"_view);
  Path no_extension("unit/source/main"_view);
  Path hidden("unit/source/.main"_view);

  EXPECT_TEXT(path.get_extension(), ".ttx"_view);
  EXPECT(no_extension.get_extension().is_empty());
  EXPECT(hidden.get_extension().is_empty());
}
