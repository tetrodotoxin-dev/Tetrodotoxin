// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/errors.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;

static Validation::Harness SourceErrors = {.name = "Source::Errors"_view};

PERIMORTEM_UNIT_TEST(SourceErrors, copied_report) {
  Errors errors;
  {
    U8 text[] = {'a', 'b', 'c'};
    U8 message[] = {'b', 'a', 'd'};
    U8 hint[] = {'f', 'i', 'x'};
    const Anchor anchor(
        Ttx::Concept::Abstract(ttx_none()), Range(0, 3), Range(1, 1));
    errors.report(
        "fixture.ttx"_view, Core::View::Bytes(text), anchor,
        Core::View::Bytes(message), Core::View::Bytes(hint));
    text[0] = 'z';
    message[0] = 'z';
    hint[0] = 'z';
  }

  // Stack text, messages and hints have all expired. The diagnostic owner
  // retains the bytes used for both inspection and delayed rendering.
  ASSERT_EQ(errors.get_size(), Count(1));
  const auto report = errors.get_error(0);
  ASSERT(report);
  EXPECT(report->get_message() == "bad"_view);
  EXPECT(report->get_hint() == "fix"_view);
  Memory::Allocator::Arena arena;
  const auto rendered = errors.render_message(arena, 0);
  EXPECT((Core::Algorithm::search(rendered, "abc"_view) != Count(-1)));
  EXPECT((
      Core::Algorithm::search(rendered, "fixture.ttx:1:2:"_view) != Count(-1)));
  EXPECT((Core::Algorithm::search(rendered, "Note: fix"_view) != Count(-1)));
  EXPECT_NOT(errors.get_error(1));
}

PERIMORTEM_UNIT_TEST(SourceErrors, revised_snapshot) {
  Errors errors;
  const Anchor anchor(Ttx::Concept::Abstract(ttx_none()), Range(0, 3));
  errors.report("same.ttx"_view, "old"_view, anchor, "first"_view);
  errors.report("same.ttx"_view, "new"_view, anchor, "second"_view);

  // A physical path does not identify immutable contents. Keeping reports
  // from two observations must not reuse the first observation's spelling.
  Memory::Allocator::Arena arena;
  EXPECT(
      (Core::Algorithm::search(errors.render_message(arena, 0), "old"_view) !=
       Count(-1)));
  EXPECT(
      (Core::Algorithm::search(errors.render_message(arena, 1), "new"_view) !=
       Count(-1)));
}

PERIMORTEM_UNIT_TEST(SourceErrors, caret_and_general) {
  Errors errors;
  const Anchor caret(
      Ttx::Concept::Abstract(ttx_none()), Range(0, 0), Range(0, 0));
  errors.report("empty.ttx"_view, Core::View::Bytes(), caret, "insert"_view);
  errors.report("empty.ttx"_view, Core::View::Bytes(), {}, "general"_view);

  Memory::Allocator::Arena arena;
  const auto insertion = errors.render_message(arena, 0);
  const auto general = errors.render_message(arena, 1);
  EXPECT(
      (Core::Algorithm::search(insertion, "empty.ttx:1:1:"_view) != Count(-1)));
  EXPECT((Core::Algorithm::search(insertion, "^"_view) != Count(-1)));
  EXPECT_NOT((Core::Algorithm::search(general, "^"_view) != Count(-1)));
  EXPECT_NOT(errors.get_error(1)->get_anchor());
}

PERIMORTEM_UNIT_TEST(SourceErrors, multiline_focus) {
  Errors errors;
  const auto origin = Ttx::Concept::Abstract(ttx_none());
  const auto text = "x\nabc\ndef"_view;
  errors.report(
      "lines.ttx"_view, text, Anchor(origin, Range(2, 7), Range(6, 1)),
      "expression"_view);
  errors.report(
      "lines.ttx"_view, text, Anchor(origin, Range(2, 7), Range(0, 1)),
      "external focus"_view);

  // Line numbers come from the retained bytes. An external focus still
  // preserves its provenance, but the renderer omits a misleading caret.
  Memory::Allocator::Arena arena;
  const auto focused = errors.render_message(arena, 0);
  const auto external = errors.render_message(arena, 1);
  EXPECT(Core::Algorithm::search(focused, "lines.ttx:3:1:"_view) != Count(-1));
  EXPECT(Core::Algorithm::search(focused, "    2 | abc\n"_view) != Count(-1));
  EXPECT(
      Core::Algorithm::search(focused, "    3 | def\n      | ^\n"_view) !=
      Count(-1));
  EXPECT(Core::Algorithm::search(external, "lines.ttx:2:1:"_view) != Count(-1));
  EXPECT(Core::Algorithm::search(external, "^"_view) == Count(-1));
}
