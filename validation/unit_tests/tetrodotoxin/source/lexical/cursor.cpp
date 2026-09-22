// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/source/dialect.hpp"
#include "tetrodotoxin/source/lexical/cursors/stream.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "validation/unit_tests/tetrodotoxin/source/lexical/publication.h"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
static Validation::Harness CursorTests = {
  .name = "Source::Lexical::Cursor"_view};

PERIMORTEM_UNIT_TEST(CursorTests, compact_value) {
  static_assert(sizeof(Token) == 8);
  static_assert(sizeof(Cursor::Api) == 2 * sizeof(void*) + sizeof(U64));
  const Token maximum(TETRODOTOXIN_TOKEN_LOCATOR_MAX, Code::Type::Unknown);
  EXPECT_EQ(maximum.get_locator(), U64(TETRODOTOXIN_TOKEN_LOCATOR_MAX));
  EXPECT_EQ(U8(maximum.value), U8(TETRODOTOXIN_TOKEN_UNKNOWN));
  EXPECT_NOT(Token());
  EXPECT(
      Token(4, Code::Type::Addressable) != Token(5, Code::Type::Addressable));
}

PERIMORTEM_UNIT_TEST(CursorTests, shared_table) {
  Memory::Allocator::Arena arena;
  Tokenizer input(arena, "one two"_view, "test.ttx"_view);
  Errors errors;
  Cursors::Stream first(input, errors);
  Cursors::Stream second(input, errors);
  auto left = first.get_interface();
  const auto right = second.get_interface();

  // Providers share their immutable table. Each Cursor carries its own index,
  // so consuming one record leaves the other at its original observation.
  EXPECT(left.get_abi().operations == right.get_abi().operations);
  EXPECT(left.get_abi().source != right.get_abi().source);
  left.consume();
  EXPECT(left.get_text() == "two"_view);
  EXPECT(right.get_text() == "one"_view);
}

PERIMORTEM_UNIT_TEST(CursorTests, forked_position) {
  Memory::Allocator::Arena arena;
  Tokenizer input(arena, "one two"_view, "test.ttx"_view);
  Errors errors;
  Cursors::Stream state(input, errors);
  auto cursor = state.get_interface();

  // Fork after observing the current Token. The copy inherits that cached
  // value but owns its progress; both continue borrowing the same spelling.
  const Token first = cursor.current();
  const auto text = cursor.get_text(first);
  auto nested = cursor;
  EXPECT(nested.consume() == first);
  EXPECT(cursor.current() == first);
  EXPECT_EQ(cursor.get_index(), U64(0));
  EXPECT(nested.get_text() == "two"_view);
  const Token second = nested.consume();
  EXPECT(nested.matches(Code::Type::Terminal));
  EXPECT(cursor.get_text() == "one"_view);

  // Forking does not create a diagnostic transaction. A report from the
  // branch remains visible even before the caller adopts its position.
  nested.create_token_error("Missing value."_view);
  EXPECT_EQ(cursor.get_error_count(), Count(1));
  EXPECT(cursor.get_error(0)->get_message() == "Missing value."_view);

  // The caller decides whether to adopt the fork. That assignment also clears
  // the old current-token cache, including when moving back to an earlier
  // index.
  cursor.set_index(nested.get_index());
  EXPECT(cursor.matches(Code::Type::Terminal));
  EXPECT(text == "one"_view);
  EXPECT(cursor.get_text(first) == "one"_view);
  EXPECT(cursor.peek(-1) == second);
  const auto end = cursor.consume();
  EXPECT(cursor.current() == end);
  EXPECT_EQ(cursor.get_index(), U64(2));
  EXPECT_EQ(cursor.get_anchor(end).get_extent().get_offset(), U64(7));
  cursor.set_index(0);
  EXPECT(cursor.current() == first);
}

PERIMORTEM_UNIT_TEST(CursorTests, positioned_lookahead) {
  Memory::Allocator::Arena arena;
  Tokenizer input(arena, "one two three"_view, "test.ttx"_view);
  Errors errors;
  Cursors::Stream provider(input, errors);
  auto cursor = provider.get_interface();
  cursor.set_index(1);

  EXPECT(cursor.get_text() == "two"_view);
  EXPECT(cursor.get_text(cursor.peek(-1)) == "one"_view);
  EXPECT(cursor.get_text(cursor.peek(1)) == "three"_view);
  EXPECT_EQ(cursor.get_index(), U64(1));

  // The raw provider takes an absolute index, independent of the position
  // carried by any facade. Looking beyond input observes the same Terminal.
  const auto api = cursor.get_abi();
  EXPECT(api.operations->get_token(api.source, 0) == cursor.peek(-1));
  EXPECT(api.operations->get_token(api.source, U64(-1)) == cursor.peek(2));
  EXPECT_EQ(provider.get_interface().get_index(), U64(0));
}

PERIMORTEM_UNIT_TEST(CursorTests, foreign_progress) {
  Memory::Allocator::Arena arena;
  Tokenizer input(arena, "one two"_view, "test.ttx"_view);
  Errors errors;
  Cursors::Stream provider(input, errors);
  auto cursor = provider.get_interface();
  const Dialect dialect(source_rejecting_dialect());

  // C advances the borrowed record and then rejects its interpretation. The
  // C++ facade must discard its cached "one" even though no graph was returned.
  EXPECT(cursor.get_text() == "one"_view);
  dialect.interpret(cursor, Ttx::Concept::Abstract(ttx_none()))
      .visit(
          [&](Ttx::Semantic::Ownership::Publication&) { EXPECT(False); },
          [&](Ttx::Semantic::Negotiation::Binding::Failure failure) {
            EXPECT(
                failure ==
                Ttx::Semantic::Negotiation::Binding::Failure::Rejected);
          });
  EXPECT_EQ(cursor.get_index(), U64(1));
  EXPECT(cursor.get_text() == "two"_view);
}

PERIMORTEM_UNIT_TEST(CursorTests, source_ranges) {
  Memory::Allocator::Arena arena;
  Tokenizer input(arena, "5 == true"_view, "test.ttx"_view);
  Errors errors;
  Cursors::Stream state(input, errors);
  auto cursor = state.get_interface();
  const auto first = cursor.consume();
  const auto operation = cursor.consume();
  const auto last = cursor.consume();
  const auto anchor = cursor.get_anchor(Span(last, first), operation);
  EXPECT_EQ(anchor.get_extent().get_size(), U64(9));
  ASSERT(anchor.get_focus());
  EXPECT_EQ(anchor.get_focus()->get_offset(), U64(2));
  EXPECT_EQ(anchor.get_focus()->get_size(), U64(2));
  cursor.report(anchor, "Bad expression."_view, "Use a value."_view);
  const auto rendered = errors.render_message(arena, 0);
  EXPECT(Core::Algorithm::search(rendered, "test.ttx:1:3:"_view) != Count(-1));
  EXPECT(Core::Algorithm::search(rendered, "^-\n"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(CursorTests, diagnostic_copy) {
  Errors errors;
  {
    Memory::Allocator::Arena arena;
    Tokenizer input(arena, "value"_view, "inner.ttx"_view);
    Cursors::Stream state(input, errors);
    const auto cursor = state.get_interface();
    U8 message[] = {'b', 'a', 'd'};
    cursor.create_token_error(Core::View::Bytes(message), "hint"_view);
    message[0] = 'x';
    EXPECT_EQ(cursor.get_error_count(), Count(1));
    EXPECT(cursor.get_error(0)->get_message() == "bad"_view);
    EXPECT_NOT(cursor.get_error(1));
  }

  Memory::Allocator::Arena arena;
  const auto rendered = errors.render_message(arena, 0);
  EXPECT(Core::Algorithm::search(rendered, "inner.ttx:1:1:"_view) != Count(-1));
  EXPECT(Core::Algorithm::search(rendered, "value"_view) != Count(-1));
  EXPECT(Core::Algorithm::search(rendered, "Note: hint"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(CursorTests, require_and_recover) {
  Memory::Allocator::Arena arena;
  Tokenizer input(arena, "wrong; next }"_view, "test.ttx"_view);
  Errors errors;
  Cursors::Stream state(input, errors);
  auto cursor = state.get_interface();
  EXPECT_NOT(cursor.require(Code::Type::Type, "Expected a type."_view));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(cursor.get_error(0)->get_message() == "Expected a type."_view);
  EXPECT(!cursor.get_error(0)->get_hint().is_empty());
  cursor.recover_to_statement();
  EXPECT(cursor.get_text() == "next"_view);
  EXPECT(cursor.require(Code::Type::Addressable));
  cursor.recover_to_scoped_statement();
  EXPECT(cursor.matches(Code::Type::ScopeEnd));
}

PERIMORTEM_UNIT_TEST(CursorTests, wide_coordinates) {
  Memory::Dynamic::Bytes text;
  text.append(' ', 65536);
  text.concat("//"_view);
  text.append('a', 8192);
  Memory::Allocator::Arena arena;
  Tokenizer input(arena, text, "large.ttx"_view);
  Errors errors;
  Cursors::Stream state(input, errors);
  auto cursor = state.get_interface();
  const auto token = cursor.consume();

  // The public value stays eight bytes. Both formerly truncating facts now
  // come from the provider's full source range rather than the Token bits.
  EXPECT_EQ(cursor.get_anchor(token).get_extent().get_offset(), U64(65536));
  EXPECT_EQ(cursor.get_text(token).get_size(), Count(8194));
  EXPECT_EQ(
      cursor.get_anchor(cursor.current()).get_extent().get_offset(),
      text.get_size());
}

PERIMORTEM_UNIT_TEST(CursorTests, unfinished_escape) {
  Memory::Allocator::Arena arena;
  const auto text = "\"abc\\"_view;
  Tokenizer input(arena, text, "test.ttx"_view);
  EXPECT_EQ(input.get_size(), Count(2));
  EXPECT(input.get_text(input[0]) == text);
  EXPECT_EQ(input.get_extent(input[1]).get_offset(), text.get_size());
}
