// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/diagnostics/suggestions.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness TetrodotoxinDiagnostics = {
  .name = "Tetrodotoxin::Diagnostics"_view,
};

PERIMORTEM_UNIT_TEST(TetrodotoxinDiagnostics, closest_name) {
  constexpr View::Bytes candidates[] = {
    "seconx"_view,
    "second"_view,
    "unrelated"_view,
  };
  Allocator::Arena arena;
  View::Bytes suggestion =
      Tetrodotoxin::Diagnostics::Suggestions::possible_candidate(
          arena, "second3"_view, candidates);
  EXPECT_TEXT(suggestion, "Did you mean `second`?"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinDiagnostics, first_distance_one) {
  constexpr View::Bytes candidates[] = {
    "secondx"_view,
    "secondy"_view,
  };
  Allocator::Arena arena;
  View::Bytes suggestion =
      Tetrodotoxin::Diagnostics::Suggestions::possible_candidate(
          arena, "second3"_view, candidates);
  EXPECT_TEXT(suggestion, "Did you mean `secondx`?"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinDiagnostics, rejected_name) {
  constexpr View::Bytes distant[] = {"alpha"_view, "value"_view};
  Allocator::Arena arena;
  EXPECT(
      Tetrodotoxin::Diagnostics::Suggestions::possible_candidate(
          arena, "omega"_view, distant)
          .is_empty());

  // Names of four bytes or fewer only tolerate one edit.
  constexpr View::Bytes short_names[] = {"bd"_view};
  EXPECT(
      Tetrodotoxin::Diagnostics::Suggestions::possible_candidate(
          arena, "ac"_view, short_names)
          .is_empty());
}

PERIMORTEM_UNIT_TEST(TetrodotoxinDiagnostics, long_names) {
  Static::Bytes<300> expected;
  Static::Bytes<300> misspelled;
  for (Count i = 0; i < expected.get_size(); i++) {
    expected[i] = 'a';
    misspelled[i] = 'a';
  }

  expected[expected.get_size() - 1] = 'x';
  misspelled[misspelled.get_size() - 1] = 'y';

  Allocator::Arena arena;
  Static::Vector<View::Bytes, 1> candidates = {{expected}};
  View::Bytes suggestion =
      Tetrodotoxin::Diagnostics::Suggestions::possible_candidate(
          arena, misspelled, candidates);

  Managed::Bytes expected_hint(arena);
  expected_hint.concat("Did you mean `"_view);
  expected_hint.concat(expected);
  expected_hint.concat("`?"_view);
  EXPECT_TEXT(suggestion, expected_hint);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinDiagnostics, transposition) {
  constexpr View::Bytes candidates[] = {"first"_view};
  Allocator::Arena arena;
  View::Bytes suggestion =
      Tetrodotoxin::Diagnostics::Suggestions::possible_candidate(
          arena, "frist"_view, candidates);
  EXPECT_TEXT(suggestion, "Did you mean `first`?"_view);
}
