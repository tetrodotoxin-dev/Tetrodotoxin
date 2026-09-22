// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/publisher.hpp"

#include "validation/unit_test.hpp"

#include <cstdio>
#include <cstdlib>
#include <ftw.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/language/product.hpp"
#include "tetrodotoxin/source/context.hpp"
#include "tetrodotoxin/source/layouts/fluid.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness PufferPublisher = {
  .name = "Puffer::Publisher"_view,
};

static auto remove_entry(const char* path, const struct stat*, S32, struct FTW*)
    -> S32 {
  return remove(path);
}

static auto publish(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes root,
    Core::View::Vector<Reference<const Abstract>> products) -> Bool {
  Tetrodotoxin::Source::Layouts::Fluid layout(products);
  Tetrodotoxin::Source::Context context(arena);
  return Puffer::Publisher(root).publish(context.pack(layout));
}

PERIMORTEM_UNIT_TEST(PufferPublisher, complete_pack_commit) {
  char root[] = "/tmp/puffer-publisher-XXXXXX";
  ASSERT(mkdtemp(root) != nullptr);
  Core::View::Bytes root_view = Core::NullTerminated::to_view(root);
  Memory::Allocator::Arena arena;
  auto& first = Tetrodotoxin::Language::Product::create(
      arena, "Example.Package/1.0/first.bin"_view, "first"_view);
  Reference<const Abstract> first_product[] = {first};
  ASSERT(publish(arena, root_view, first_product));
  Memory::Managed::Bytes first_path(arena, root_view);
  first_path.concat("/Example.Package/1.0/first.bin"_view);
  auto first_bytes = System::File::read(first_path.get_view());
  ASSERT(first_bytes);
  EXPECT_TEXT(*first_bytes, "first"_view);

  auto& second = Tetrodotoxin::Language::Product::create(
      arena, "Example.Package/1.0/second.bin"_view, "second"_view);
  Reference<const Abstract> second_product[] = {second};
  ASSERT(publish(arena, root_view, second_product));
  EXPECT_NOT(System::File::read(first_path.get_view()));
  Memory::Managed::Bytes second_path(arena, root_view);
  second_path.concat("/Example.Package/1.0/second.bin"_view);
  auto second_bytes = System::File::read(second_path.get_view());
  ASSERT(second_bytes);
  EXPECT_TEXT(*second_bytes, "second"_view);

  nftw(root, remove_entry, 32, FTW_DEPTH | FTW_PHYS);
}

PERIMORTEM_UNIT_TEST(PufferPublisher, rejects_escape_and_collision) {
  char root[] = "/tmp/puffer-publisher-invalid-XXXXXX";
  ASSERT(mkdtemp(root) != nullptr);
  Core::View::Bytes root_view = Core::NullTerminated::to_view(root);
  Memory::Allocator::Arena arena;
  auto& escaping = Tetrodotoxin::Language::Product::create(
      arena, "Example.Package/1.0/../escape"_view, {});
  Reference<const Abstract> escaped[] = {escaping};
  EXPECT_NOT(publish(arena, root_view, escaped));

  auto& first = Tetrodotoxin::Language::Product::create(
      arena, "Example.Package/1.0/api.hpp"_view, "first"_view);
  auto& second = Tetrodotoxin::Language::Product::create(
      arena, "Example.Package/1.0/api.hpp"_view, "second"_view);
  Reference<const Abstract> collided[] = {first, second};
  EXPECT_NOT(publish(arena, root_view, collided));
  nftw(root, remove_entry, 32, FTW_DEPTH | FTW_PHYS);
}
