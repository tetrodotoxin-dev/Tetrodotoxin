// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialects/source/associations.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/dialects/source/tokenizer.hpp"
#include "toolchain/validation/unit_test.hpp"
#include "ttx/concept/abstract.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Dialects;
using namespace Toolchain::Validation;

static Harness associations_tests("Source::Associations");

// Distinct native subjects provide stable identities without depending on a
// concrete language model. Associations only borrows these subjects.
class Subject {
 public:
  auto get_data() const -> View::Bytes { return View::Bytes(); }
};

// An exact token focus wins over a containing span, while uncovered bytes
// and unknown identities have no association. Looking up a fresh Abstract
// view of the same subject must find its original anchor.
VALIDATION_TEST(associations_tests, precise_selection) {
  Allocator::Arena arena;
  Source::Tokenizer tokenizer(
      arena, "outer inner end"_view, "association.ttx"_view);
  Source::Associations associations(arena);
  const Source::Token* tokens = tokenizer.get_tokens().get_data();
  const Subject outer_owner;
  const Subject inner_owner;
  const Subject missing_owner;
  const auto outer = Ttx::Concept::Abstract::provide(outer_owner);
  const auto inner = Ttx::Concept::Abstract::provide(inner_owner);
  const auto missing_semantic = Ttx::Concept::Abstract::provide(missing_owner);

  associations.create(
      Source::Anchor::create(tokens[0], Source::Span(tokens[0], tokens[2])),
      Ttx::Concept::Abstract::provide(outer_owner));
  associations.create(
      Source::Anchor::create(Source::Span(tokens[1])),
      Ttx::Concept::Abstract::provide(inner_owner));

  auto focused = associations.find_at(tokens[1].get_offset());
  auto containing = associations.find_at(tokens[2].get_offset());
  auto missing = associations.find_at(tokenizer.get_source_text().get_size());
  auto outer_anchor = associations.find(outer);
  auto inner_anchor = associations.find(inner);

  ASSERT(focused);
  ASSERT(containing);
  ASSERT(outer_anchor);
  ASSERT(inner_anchor);
  EXPECT_EQ(focused->get_identity(), inner.get_identity());
  EXPECT_EQ(containing->get_identity(), outer.get_identity());
  EXPECT_EQ(outer_anchor->get_token().get_offset(), tokens[0].get_offset());
  EXPECT_EQ(inner_anchor->get_token().get_offset(), tokens[1].get_offset());
  EXPECT_NOT(missing);
  EXPECT_NOT(associations.find(missing_semantic));
}
