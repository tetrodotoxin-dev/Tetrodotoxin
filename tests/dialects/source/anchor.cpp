// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialects/source/anchor.hpp"

#include "toolchain/validation/unit_test.hpp"

using namespace Tetrodotoxin::Dialects::Source;
using namespace Toolchain::Validation;

static Harness anchor_tests("Source::Anchor");

// Anchor separates the diagnostic focus from its containing Span. Default,
// empty and external focus values must preserve that range, and reversed or
// nested ranges must combine without shrinking it.
VALIDATION_TEST(anchor_tests, focus_contract) {
  Token first(0, 1, 1, 1, Code::Type::Numeric);
  Token operation(4, 1, 5, 1, Code::Type::CmpOp);
  Token middle(4, 1, 5, 1, Code::Type::Numeric);
  Token last(8, 1, 9, 1, Code::Type::Numeric);
  Span outer(first, last);
  Span inner(middle);
  Anchor defaulted = Anchor::create(outer);
  Anchor focused = Anchor::create(operation, outer);
  Anchor empty = Anchor::create(Token(), outer);
  Token external(40, 8, 2, 1, Code::Type::AddOp);
  Anchor outside = Anchor::create(external, outer);

  Anchor reversed = Anchor::create(middle, Span(last), Span(first));
  Anchor nested = Anchor::create(middle, outer, inner);

  EXPECT_EQ(defaulted.get_token().get_offset(), first.get_offset());
  EXPECT_EQ(defaulted.get_span().get_offset(), outer.get_offset());
  EXPECT_EQ(defaulted.get_span().get_size(), outer.get_size());

  EXPECT_EQ(focused.get_token().get_offset(), operation.get_offset());
  EXPECT(focused.get_token().get_code() == Code::Type::CmpOp);
  EXPECT_EQ(focused.get_span().get_offset(), outer.get_offset());
  EXPECT_EQ(focused.get_span().get_size(), outer.get_size());

  EXPECT_NOT(empty.get_token());
  EXPECT_EQ(empty.get_span().get_offset(), outer.get_offset());
  EXPECT_EQ(empty.get_span().get_size(), outer.get_size());

  EXPECT_EQ(outside.get_token().get_offset(), external.get_offset());
  EXPECT_EQ(outside.get_span().get_offset(), outer.get_offset());
  EXPECT_EQ(outside.get_span().get_size(), outer.get_size());

  EXPECT(reversed.get_span().get_offset() == first.get_offset());
  EXPECT(reversed.get_span().get_size() == outer.get_size());
  EXPECT(nested.get_span().get_offset() == outer.get_offset());
  EXPECT(nested.get_span().get_size() == outer.get_size());
}
