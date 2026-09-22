// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/parser/comment.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Language;
using namespace Ttx;
using namespace Validation;

static Harness ParserCommentTests = {
  .name = "Tetrodotoxin::Language::Parser::Comment"_view,
};

PERIMORTEM_UNIT_TEST(ParserCommentTests, greedy) {
  Allocator::Arena arena;
  Tetrodotoxin::Source::Lexical::Errors errors;
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(
      arena, "// First line\n// Second line\nValue"_view,
      "<greedy comments>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Tetrodotoxin::Source::Lexical::Cursor cursor(tokenizer, errors, associations);

  Option<const Tetrodotoxin::Source::Documentation&> documentation =
      Parser::Comment::parse(cursor);
  Bool correct = documentation.visit(
      []() { return False; },
      [](const Tetrodotoxin::Source::Documentation& selected) -> Bool {
        return Bool(
            selected.line_count() == 2 &&
            selected.get_line(0) == "First line"_view &&
            selected.get_line(1) == "Second line"_view);
      });

  EXPECT(correct);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Tetrodotoxin::Source::Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Value"_view);
}

PERIMORTEM_UNIT_TEST(ParserCommentTests, preserves_empty) {
  Allocator::Arena arena;
  Tetrodotoxin::Source::Lexical::Errors errors;
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(
      arena, "// First line\n//\n// \n//  Indented line\nValue"_view,
      "<empty comment lines>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Tetrodotoxin::Source::Lexical::Cursor cursor(tokenizer, errors, associations);

  Option<const Tetrodotoxin::Source::Documentation&> documentation =
      Parser::Comment::parse(cursor);
  Bool correct = documentation.visit(
      []() { return False; },
      [](const Tetrodotoxin::Source::Documentation& selected) -> Bool {
        return Bool(
            selected.line_count() == 4 &&
            selected.get_line(0) == "First line"_view &&
            selected.get_line(1).is_empty() &&
            selected.get_line(2).is_empty() &&
            selected.get_line(3) == " Indented line"_view);
      });

  EXPECT(correct);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Tetrodotoxin::Source::Lexical::Code::Type::Type));
}

PERIMORTEM_UNIT_TEST(ParserCommentTests, excludes_raw_comments) {
  Allocator::Arena arena;
  Tetrodotoxin::Source::Lexical::Errors errors;
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(
      arena,
      "/// Tetrodotoxin\n"
      "/// Copyright metadata\n"
      "// Public documentation.\n"
      "Value"_view,
      "<raw comments>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Tetrodotoxin::Source::Lexical::Cursor cursor(tokenizer, errors, associations);

  const Tetrodotoxin::Source::Documentation& documentation = Parser::Comment::parse(cursor);
  EXPECT_EQ(documentation.line_count(), Count(1));
  EXPECT_TEXT(documentation.get_line(0), "Public documentation."_view);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Tetrodotoxin::Source::Lexical::Code::Type::Type));
}

PERIMORTEM_UNIT_TEST(ParserCommentTests, raw_only_is_not_documentation) {
  Allocator::Arena arena;
  Tetrodotoxin::Source::Lexical::Errors errors;
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(
      arena, "/// Build metadata\nValue"_view, "<raw only>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Tetrodotoxin::Source::Lexical::Cursor cursor(tokenizer, errors, associations);

  const Tetrodotoxin::Source::Documentation& documentation = Parser::Comment::parse(cursor);
  EXPECT(documentation.is_empty());
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Tetrodotoxin::Source::Lexical::Code::Type::Type));
}
