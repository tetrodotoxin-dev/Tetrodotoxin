// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/associations.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "ttx/concept/answers/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx;
using namespace Validation;

// Associations selects authored coordinates and keeps the supplied identity.
// Its fixture needs no alias resolution or native type behavior.
class Subject {
 public:
  explicit Subject(View::Bytes name) : name(name) {}
  auto get_data() const -> View::Bytes { return name; }

 private:
  View::Bytes name;
};

static Harness TtxLexicalAssociations = {
  .name = "TTX::Lexical::Associations"_view,
};

PERIMORTEM_UNIT_TEST(TtxLexicalAssociations, precise_selection) {
  Allocator::Arena arena;
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(
      arena, "outer inner end"_view, "association.ttx"_view);
  Tetrodotoxin::Source::Associations associations(arena);
  const auto& tokens = tokenizer;
  Subject outer("Outer"_view);
  Subject inner("Inner"_view);

  associations.create(
      tokenizer.get_anchor(
          Tetrodotoxin::Source::Lexical::Span(tokens[0], tokens[2]), tokens[0]),
      Concept::Abstract::provide(outer));
  associations.create(
      tokenizer.get_anchor(
          Tetrodotoxin::Source::Lexical::Span(tokens[1]), tokens[1]),
      Concept::Abstract::provide(inner));

  auto focused =
      associations.find_at(tokenizer.get_extent(tokens[1]).get_offset());
  auto containing =
      associations.find_at(tokenizer.get_extent(tokens[2]).get_offset());
  auto missing = associations.find_at(tokenizer.get_source_text().get_size());
  auto outer_anchor = associations.find(Concept::Abstract::provide(outer));
  auto inner_anchor = associations.find(Concept::Abstract::provide(inner));

  ASSERT(focused);
  ASSERT(containing);
  ASSERT(outer_anchor);
  ASSERT(inner_anchor);
  EXPECT(*focused == Concept::Abstract::provide(inner));
  EXPECT(*containing == Concept::Abstract::provide(outer));
  EXPECT(
      outer_anchor->get_focus()->get_offset() ==
      tokenizer.get_extent(tokens[0]).get_offset());
  EXPECT(
      inner_anchor->get_focus()->get_offset() ==
      tokenizer.get_extent(tokens[1]).get_offset());
  EXPECT_NOT(missing);
  EXPECT_NOT(associations.find(Ttx::Concept::Answers::Unknown::get_unknown()));
}
