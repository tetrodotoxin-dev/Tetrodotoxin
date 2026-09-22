// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source::Lexical;
static Validation::Harness TtxAnchor = {.name = "Source::Lexical::Anchor"_view};

PERIMORTEM_UNIT_TEST(TtxAnchor, focus_contract) {
  Memory::Allocator::Arena arena;
  const Tokenizer tokenizer(arena, "5 == true"_view, "test.ttx"_view);
  const auto first = tokenizer[0];
  const auto operation = tokenizer[1];
  const auto last = tokenizer[2];
  const auto focused = tokenizer.get_anchor(Span(first, last), operation);
  const auto reversed = tokenizer.get_anchor(Span(last, first));
  EXPECT_EQ(focused.get_extent().get_offset(), U64(0));
  EXPECT_EQ(focused.get_extent().get_size(), U64(9));
  ASSERT(focused.get_focus());
  EXPECT_EQ(focused.get_focus()->get_offset(), U64(2));
  EXPECT_EQ(focused.get_focus()->get_size(), U64(2));
  EXPECT_EQ(reversed.get_extent().get_offset(), U64(0));
  EXPECT_EQ(reversed.get_extent().get_size(), U64(9));
  EXPECT_NOT(reversed.get_focus());
}
