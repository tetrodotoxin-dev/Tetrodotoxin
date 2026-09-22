// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness ManagedBytes = {
  .name = "Managed::Bytes"_view,
};

PERIMORTEM_UNIT_TEST(ManagedBytes, bounds) {
  Allocator::Arena arena;
  Managed::Bytes bytes(arena, "abc"_view);

  EXPECT_EQ(bytes[2], U8('c'));
  EXPECT_EQ(bytes[3], U8(0));
  EXPECT_EQ(bytes.at(Count(-1)), U8(0));
}
