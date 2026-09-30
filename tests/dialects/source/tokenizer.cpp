// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialects/source/tokenizer.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/dialects/source/cursor.hpp"
#include "tetrodotoxin/dialects/source/errors.hpp"
#include "tetrodotoxin/dialects/source/lexicon.hpp"
#include "tetrodotoxin/dialects/source/span.hpp"
#include "toolchain/validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Dialects::Source;
using namespace Toolchain::Validation;

static Harness tokenizer_tests("Source::Tokenizer");

// The longest access spelling must win without absorbing an adjacent name.
// Mixing qualified names, indexing, swizzles and slices catches dispatch
// that confuses their shared punctuation.
VALIDATION_TEST(tokenizer_tests, access_operators) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "Perimortem.Graphics color.[r, g] color:[start, 2] "
      "layout[Type] value.member"_view,
      "Test.Package"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  const auto* token_data = tokens.get_data();
  ASSERT_EQ(tokens.get_size(), Count(23));

  EXPECT(token_data[0].get_code() == Code::Type::Type);
  EXPECT(token_data[1].get_code() == Code::Type::AddressOp);
  EXPECT_TEXT(token_data[1].caculate_text(source), "."_view);
  EXPECT(token_data[2].get_code() == Code::Type::Type);

  EXPECT_TEXT(token_data[4].caculate_text(source), ".["_view);
  EXPECT(token_data[4].get_code() == Code::Type::SwizzleOp);
  EXPECT_TEXT(token_data[10].caculate_text(source), ":["_view);
  EXPECT(token_data[10].get_code() == Code::Type::ValueAccessOp);

  EXPECT_TEXT(token_data[16].caculate_text(source), "["_view);
  EXPECT(token_data[16].get_code() == Code::Type::BracketStart);
  EXPECT_TEXT(token_data[20].caculate_text(source), "."_view);
  EXPECT(token_data[20].get_code() == Code::Type::AddressOp);
}

// Division, comments, calls and comparisons share slash or angle
// characters. Their neighboring forms must stay distinct, including the
// retired disabled declaration spelling, which remains two ordinary
// operators.
VALIDATION_TEST(tokenizer_tests, division_tokens) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "/> // retained comment\nleft / right -> call value > other >= floor"_view,
      "Test.Package"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  const auto* token_data = tokens.get_data();
  ASSERT_EQ(tokens.get_size(), Count(14));

  EXPECT(token_data[0].get_code() == Code::Type::DivOp);
  EXPECT_TEXT(token_data[0].caculate_text(source), "/"_view);
  EXPECT(token_data[1].get_code() == Code::Type::GreaterOp);
  EXPECT_TEXT(token_data[1].caculate_text(source), ">"_view);
  EXPECT(token_data[2].get_code() == Code::Type::Comment);
  EXPECT(token_data[4].get_code() == Code::Type::DivOp);
  EXPECT(token_data[6].get_code() == Code::Type::CallOp);
  EXPECT(token_data[9].get_code() == Code::Type::GreaterOp);
  EXPECT(token_data[11].get_code() == Code::Type::GreaterEqOp);
  EXPECT(token_data[13].get_code() == Code::Type::Terminal);
}

// Raw comments retain their full authored prefix while ordinary comments
// remain documentation tokens. This distinction lets formatting preserve
// metadata without treating it as public prose.
VALIDATION_TEST(tokenizer_tests, raw_comments) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena, "/// Tetrodotoxin\n// Public documentation.\n//// metadata"_view,
      "Test.Package"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  ASSERT_EQ(tokens.get_size(), Count(4));
  EXPECT(tokens[0].get_code() == Code::Type::RawComment);
  EXPECT_TEXT(tokens[0].caculate_text(source), "/// Tetrodotoxin"_view);
  EXPECT(tokens[1].get_code() == Code::Type::Comment);
  EXPECT_TEXT(tokens[1].caculate_text(source), "// Public documentation."_view);
  EXPECT(tokens[2].get_code() == Code::Type::RawComment);
  EXPECT_TEXT(tokens[2].caculate_text(source), "//// metadata"_view);
  EXPECT(tokens[3].get_code() == Code::Type::Terminal);
}

// The propagation marker remains a separate token after a value. Repeating
// it on the next line checks that its byte location follows source
// coordinates rather than the preceding token.
VALIDATION_TEST(tokenizer_tests, propagation_operator) {
  Allocator::Arena arena;
  Tokenizer tokenizer(arena, "value?\nnext?"_view, "Test.Package"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  const auto* token_data = tokens.get_data();
  ASSERT_EQ(tokens.get_size(), Count(5));
  EXPECT(token_data[0].get_code() == Code::Type::Addressable);
  EXPECT(token_data[1].get_code() == Code::Type::QuestionOp);
  EXPECT_TEXT(token_data[1].caculate_text(source), "?"_view);
  EXPECT_EQ(token_data[1].get_offset(), U32(5));
  EXPECT_EQ(token_data[1].get_line(), U32(1));
  EXPECT_EQ(token_data[1].get_column(), U32(6));
  EXPECT_EQ(token_data[1].get_size(), U32(1));
  EXPECT(token_data[2].get_code() == Code::Type::Addressable);
  EXPECT(token_data[3].get_code() == Code::Type::QuestionOp);
  EXPECT_TEXT(token_data[3].caculate_text(source), "?"_view);
  EXPECT_EQ(token_data[3].get_offset(), U32(11));
  EXPECT_EQ(token_data[3].get_line(), U32(2));
  EXPECT_EQ(token_data[3].get_column(), U32(5));
  EXPECT_EQ(token_data[3].get_size(), U32(1));
  EXPECT(token_data[4].get_code() == Code::Type::Terminal);
}

// Reserved words classify as their exact Codes, but a longer name and
// attribute payload remain authored names. This guards against keyword
// matching that consumes a prefix or classifies attribute text as a
// declaration.
VALIDATION_TEST(tokenizer_tests, reserved_keywords) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "public private expose state const enum struct object using new package "
      "emit emitter @package_name @public"_view,
      "Test.Package"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  const auto* token_data = tokens.get_data();
  static constexpr Static::Vector<Code::Type, 13> expected = {{
    Code::Type::Public,
    Code::Type::Private,
    Code::Type::Expose,
    Code::Type::State,
    Code::Type::Const,
    Code::Type::Enum,
    Code::Type::Struct,
    Code::Type::Object,
    Code::Type::Using,
    Code::Type::New,
    Code::Type::Package,
    Code::Type::Emit,
    Code::Type::Addressable,
  }};
  static constexpr Count expected_size = expected.get_size();
  ASSERT_EQ(tokens.get_size(), expected_size + 3);
  for (Count i = 0; i < expected_size; i++) {
    EXPECT(token_data[i].get_code() == expected[i]);
  }

  EXPECT(token_data[expected_size].get_code() == Code::Type::Attribute);
  EXPECT_TEXT(
      token_data[expected_size].caculate_text(source), "package_name"_view);
  EXPECT(token_data[expected_size + 1].get_code() == Code::Type::Attribute);
  EXPECT_TEXT(
      token_data[expected_size + 1].caculate_text(source), "public"_view);
  EXPECT(token_data[expected_size + 2].get_code() == Code::Type::Terminal);
}

// Attribute text omits its opening marker while its source column includes
// it. Consecutive attributes expose cumulative column drift before the
// following ordinary name.
VALIDATION_TEST(tokenizer_tests, attribute_columns) {
  Allocator::Arena arena;
  Tokenizer tokenizer(arena, "@first @second value"_view, "Test.Package"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  ASSERT_EQ(tokens.get_size(), Count(4));

  EXPECT(tokens[0].get_code() == Code::Type::Attribute);
  EXPECT_TEXT(tokens[0].caculate_text(source), "first"_view);
  EXPECT_EQ(tokens[0].get_offset(), U32(1));
  EXPECT_EQ(tokens[0].get_column(), U32(1));
  EXPECT_EQ(tokens[0].get_size(), U32(5));

  EXPECT(tokens[1].get_code() == Code::Type::Attribute);
  EXPECT_TEXT(tokens[1].caculate_text(source), "second"_view);
  EXPECT_EQ(tokens[1].get_offset(), U32(8));
  EXPECT_EQ(tokens[1].get_column(), U32(8));
  EXPECT_EQ(tokens[1].get_size(), U32(6));

  EXPECT(tokens[2].get_code() == Code::Type::Addressable);
  EXPECT_TEXT(tokens[2].caculate_text(source), "value"_view);
  EXPECT_EQ(tokens[2].get_offset(), U32(15));
  EXPECT_EQ(tokens[2].get_column(), U32(16));
  EXPECT_EQ(tokens[2].get_size(), U32(5));

  EXPECT(tokens[3].get_code() == Code::Type::Terminal);
  EXPECT_EQ(tokens[3].get_offset(), U32(20));
  EXPECT_EQ(tokens[3].get_column(), U32(21));
}

// The lexicon supplies spellings and character classes used by tokenization
// and diagnostics. Variable spellings stay empty, and the removed disabled
// declaration marker has no Code to recover.
VALIDATION_TEST(tokenizer_tests, lexicon) {
  using Token = Code::Type;

  EXPECT_TEXT(Lexicon::get_spelling(Token::TypeAccessOp), "::"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::SwizzleOp), ".["_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::ValueAccessOp), ":["_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::CallOp), "->"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::QuestionOp), "?"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::Dialect), "dialect"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::Source), "source"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::Emit), "emit"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::Expose), "expose"_view);
  for (U8 value = 0; value <= static_cast<U8>(Token::Const); value++) {
    EXPECT_NOT(Lexicon::get_spelling(Token(value)) == "/>"_view);
  }
  EXPECT(Lexicon::get_spelling(Token::Addressable).is_empty());
  EXPECT(Lexicon::get_spelling(Token::Numeric).is_empty());
  EXPECT(
      Lexicon::get_keyword("public"_view, Token::Addressable) == Token::Public);
  EXPECT(Lexicon::get_keyword("emit"_view, Token::Addressable) == Token::Emit);
  EXPECT(
      Lexicon::get_keyword("emitter"_view, Token::Addressable) ==
      Token::Addressable);
  EXPECT(
      Lexicon::get_keyword("value"_view, Token::Addressable) ==
      Token::Addressable);
  EXPECT(Lexicon::is_whitespace(' '));
  EXPECT(Lexicon::is_whitespace('\n'));
  EXPECT(Lexicon::is_whitespace('\r'));
  EXPECT(Lexicon::is_whitespace('\t'));
  EXPECT_NOT(Lexicon::is_whitespace('\0'));
  EXPECT_EQ(Lexicon::get_hex_value('0'), U8(0));
  EXPECT_EQ(Lexicon::get_hex_value('9'), U8(9));
  EXPECT_EQ(Lexicon::get_hex_value('A'), U8(10));
  EXPECT_EQ(Lexicon::get_hex_value('F'), U8(15));
  EXPECT_EQ(Lexicon::get_hex_value('a'), U8(10));
  EXPECT_EQ(Lexicon::get_hex_value('f'), U8(15));
}

// Validation accepts one complete spelling or an explicitly allowed
// qualified route. Empty segments, mismatched categories and unterminated
// literals must fail without accepting a valid prefix.
VALIDATION_TEST(tokenizer_tests, spelling_validation) {
  using Token = Code::Type;

  static constexpr Static::Vector<Token, 1> type_access = {{
    Token::TypeAccessOp,
  }};
  static constexpr Static::Vector<Token, 1> address = {{
    Token::AddressOp,
  }};
  static constexpr Static::Vector<Token, 2> qualified = {{
    Token::TypeAccessOp,
    Token::AddressOp,
  }};

  EXPECT(Lexicon::validate(Token::Type, "Type"_view));
  EXPECT(Lexicon::validate(Token::Type, "Type_Name2"_view));
  EXPECT_NOT(Lexicon::validate(Token::Type, View::Bytes()));
  EXPECT_NOT(Lexicon::validate(Token::Type, "type"_view));
  EXPECT_NOT(Lexicon::validate(Token::Type, "Type.Name"_view));

  EXPECT(Lexicon::validate(Token::Type, "Scenes::Splash"_view, type_access));
  EXPECT(Lexicon::validate(Token::Type, "Perimortem.Graphics"_view, address));
  EXPECT(Lexicon::validate(Token::Type, "Root::Child.Leaf"_view, qualified));
  EXPECT_NOT(Lexicon::validate(Token::Type, "::Splash"_view, type_access));
  EXPECT_NOT(Lexicon::validate(Token::Type, "Scenes::"_view, type_access));
  EXPECT_NOT(
      Lexicon::validate(Token::Type, "Scenes::::Splash"_view, type_access));
  EXPECT_NOT(
      Lexicon::validate(Token::Type, "Scenes :: Splash"_view, type_access));

  EXPECT(Lexicon::validate(Token::Addressable, "local_name"_view));
  EXPECT_NOT(Lexicon::validate(Token::Addressable, "public"_view));
  EXPECT(Lexicon::validate(Token::Public, "public"_view));
  EXPECT_NOT(Lexicon::validate(Token::Addressable, "emit"_view));
  EXPECT(Lexicon::validate(Token::Emit, "emit"_view));
  EXPECT(Lexicon::validate(Token::Numeric, "123"_view));
  EXPECT_NOT(Lexicon::validate(Token::Numeric, "1.2"_view));
  EXPECT(Lexicon::validate(Token::Float, "1.25"_view));
  EXPECT_NOT(Lexicon::validate(Token::Float, "1.2.5"_view));
  EXPECT(Lexicon::validate(Token::Hex, "0xAB"_view));
  EXPECT_NOT(Lexicon::validate(Token::Hex, "0x"_view));

  EXPECT(Lexicon::validate(Token::String, "\"1.0\""_view));
  EXPECT(Lexicon::validate(Token::String, "\"escaped \\\" quote\""_view));
  EXPECT_NOT(Lexicon::validate(Token::String, "\"unterminated"_view));
  EXPECT_NOT(Lexicon::validate(Token::String, "\"escaped terminal\\\""_view));
  EXPECT(Lexicon::validate(Token::Bytes, "0x[]"_view));
  EXPECT(Lexicon::validate(Token::Embedded, "$[resources/table.bin]"_view));
  EXPECT(Lexicon::validate(Token::Comment, "// text"_view));
  EXPECT_NOT(Lexicon::validate(Token::Comment, "// line\n"_view));
  EXPECT_NOT(Lexicon::validate(Token::Comment, "/// raw text"_view));
  EXPECT(Lexicon::validate(Token::RawComment, "/// raw text"_view));
  EXPECT_NOT(Lexicon::validate(Token::RawComment, "// documentation"_view));
  EXPECT(Lexicon::validate(Token::TypeAccessOp, "::"_view));
  EXPECT(Lexicon::validate(Token::QuestionOp, "?"_view));
  EXPECT_NOT(Lexicon::validate(Token::QuestionOp, "!"_view));
  EXPECT_NOT(Lexicon::validate(Token::Unknown, "?"_view));
}

// Diagnostic descriptions distinguish lexical roles that can share nearby
// syntax. Checking named roles and the Unknown fallback prevents an error
// report from substituting raw punctuation for the expected category.
VALIDATION_TEST(tokenizer_tests, code_semantics) {
  EXPECT_TEXT(
      Code(Code::Type::Public).get_semantics(),
      "public publication modifier"_view);
  EXPECT_TEXT(
      Code(Code::Type::Assign).get_semantics(), "assignment operator"_view);
  EXPECT_TEXT(
      Code(Code::Type::QuestionOp).get_semantics(),
      "propagation operator"_view);
  EXPECT_TEXT(Code(Code::Type::Emit).get_semantics(), "emission keyword"_view);
  EXPECT_TEXT(
      Code(Code::Type::Addressable).get_semantics(),
      "Addressable space name"_view);
  EXPECT_TEXT(
      Code(Code::Type::Hex).get_semantics(), "U64 hexadecimal literal"_view);
  EXPECT_TEXT(
      Code(Code::Type::RawComment).get_semantics(), "raw source comment"_view);
  EXPECT_TEXT(Code(Code::Type::Terminal).get_semantics(), "terminal Code"_view);
  EXPECT_TEXT(
      Code(Code::Type::Unknown).get_semantics(), "unknown source Code"_view);
  EXPECT_NEQ(
      Code(Code::Type::Public).get_semantics(),
      Code(Code::Type::Private).get_semantics());
}

// A hexadecimal integer must retain both its Hex Code and its complete
// authored prefix. This protects consumers that decode the token by slicing
// its original source.
VALIDATION_TEST(tokenizer_tests, hexadecimal_code) {
  Allocator::Arena arena;
  Tokenizer tokenizer(arena, "0xAB"_view, "test.ttx"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  ASSERT_EQ(tokens.get_size(), Count(2));
  EXPECT(tokens.get_data()[0].get_code() == Code::Type::Hex);
  EXPECT_TEXT(
      tokens.get_data()[0].caculate_text(tokenizer.get_source_text()),
      "0xAB"_view);
  EXPECT(tokens.get_data()[1].get_code() == Code::Type::Terminal);
}

// The terminal has zero width at the exact end of the authored bytes. A
// default token is also terminal but has no source coordinates, so parsers
// can distinguish absent evidence from the real end position.
VALIDATION_TEST(tokenizer_tests, terminal_boundary) {
  Allocator::Arena arena;
  Token empty;
  Tokenizer tokenizer(arena, "one two"_view, "test.ttx"_view);
  View::Vector<Token> tokens = tokenizer.get_tokens();
  Token terminal = tokens.get_data()[tokens.get_size() - 1];

  EXPECT(empty.get_code() == Code::Type::Terminal);
  EXPECT(empty.get_offset() == 0);
  EXPECT(empty.get_line() == 0);
  EXPECT(empty.get_column() == 0);
  EXPECT(empty.get_size() == 0);
  EXPECT(terminal.get_code() == Code::Type::Terminal);
  EXPECT(terminal.get_offset() == tokenizer.get_source_text().get_size());
  EXPECT(terminal.get_size() == 0);
}

// Consuming the final authored token reaches Terminal, and another consume
// stays there. Parser completion must not require a separate sentinel or
// advance beyond the stream.
VALIDATION_TEST(tokenizer_tests, cursor_consume) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Cursor cursor(tokenizer.get_stream());

  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "value"_view);
  cursor.consume();
  EXPECT(cursor.matches(Code::Type::Terminal));
  cursor.consume();
  EXPECT(cursor.matches(Code::Type::Terminal));
}

// Signed lookbehind identifies the last consumed token, including the final
// authored token. The resulting Span must cover completed syntax without
// absorbing the terminal or following text.
VALIDATION_TEST(tokenizer_tests, completed_span) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "one two"_view, "test.ttx"_view);
  Cursor cursor(tokenizer.get_stream());

  EXPECT(!cursor.peek(-1));
  EXPECT_TEXT(
      cursor.peek(1).caculate_text(cursor.get_source_text()), "two"_view);
  EXPECT(!cursor.peek(3));

  Token opening = cursor.consume();
  Span first(opening, cursor.peek(-1));
  cursor.consume();
  Span complete(opening, cursor.peek(-1));
  cursor.consume();
  Span at_terminal(opening, cursor.peek(-1));

  EXPECT_TEXT(first.caculate_text(cursor.get_source_text()), "one"_view);
  EXPECT_TEXT(complete.caculate_text(cursor.get_source_text()), "one two"_view);
  EXPECT_TEXT(
      at_terminal.caculate_text(cursor.get_source_text()), "one two"_view);
  EXPECT(complete.get_end().get_code() == Code::Type::Addressable);
  EXPECT(at_terminal.get_end().get_code() == Code::Type::Addressable);
  EXPECT(!cursor.peek(-3));
  EXPECT(!cursor.peek(1));
}

// A successful requirement returns the matching token, consumes it once and
// produces no diagnostic. A parser can then continue from the next token
// without repeating the match.
VALIDATION_TEST(tokenizer_tests, require_success) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Cursor cursor(tokenizer.get_stream());

  Token token =
      errors.require(cursor, Code::Type::Addressable, "Expected address."_view);

  ASSERT(token.is_valid());
  EXPECT_TEXT(token.caculate_text(cursor.get_source_text()), "value"_view);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Code::Type::Terminal));
}

// A failed requirement leaves the unexpected token available for recovery.
// Its report must retain the authored explanation and the expected versus
// actual lexical categories.
VALIDATION_TEST(tokenizer_tests, require_error) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Cursor cursor(tokenizer.get_stream());

  Token required =
      errors.require(cursor, Code::Type::Type, "Expected type."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  EXPECT_NOT(required.is_valid());
  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "Expected type."_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          rendered,
          "Expected lexical token Type space name but got Addressable space name."_view) !=
      Count(-1));
  EXPECT(Algorithm::search(rendered, "test.ttx"_view) != Count(-1));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "value"_view);
}

// Span orders reversed endpoints by source position while preserving the
// full extent across lines. Its default value must remain invalid rather
// than inventing a source range.
VALIDATION_TEST(tokenizer_tests, span_ordering) {
  Token early(4, 2, 3, 2, Code::Type::Type);
  Token late(304, 8, 12, 4, Code::Type::Type);
  Span span(late, early);
  Span invalid;

  EXPECT_NOT(invalid);
  EXPECT(span);
  EXPECT(span.get_start().get_offset() == early.get_offset());
  EXPECT(span.get_end().get_offset() == late.get_offset());
  EXPECT_EQ(span.get_line_count(), U16(7));
  EXPECT_EQ(span.get_size(), Count(304));
}

// Expression diagnostics carry the parser supplied range, message and
// recovery hint. This covers the Cursor path that reports several tokens as
// one failed construct.
VALIDATION_TEST(tokenizer_tests, range_error) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "value = 1"_view, "test.ttx"_view);
  Cursor cursor(tokenizer.get_stream());

  Token start = cursor.current();
  cursor.consume();
  cursor.consume();
  Token end = cursor.current();
  errors.create_expression_error(
      cursor, Span(start, end), "Bad expression."_view, "Use a value."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "Bad expression."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Use a value."_view) != Count(-1));
}

// A report without an authored range still identifies its source and
// explanation. Supplying a display path changes presentation without
// changing the stored source identity.
VALIDATION_TEST(tokenizer_tests, source_error) {
  Allocator::Arena render_arena;
  Errors errors;

  {
    Errors::Report report(
        errors, "test.ttx"_view, "source"_view, Anchor::create(Span()));
    report << "Bad source."_view;
    report.get_hint() << "Try again."_view;
  }

  View::Bytes rendered = errors.render_message(render_arena, 0);
  View::Bytes displayed =
      errors.render_message(render_arena, 0, "packages/example/test.ttx"_view);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "test.ttx"_view) != Count(-1));
  EXPECT(
      Algorithm::search(displayed, "packages/example/test.ttx"_view) !=
      Count(-1));
  EXPECT(Algorithm::search(rendered, "Bad source."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Try again."_view) != Count(-1));
}

// Report publishes only a nonempty message at scope exit and retains the
// text needed for later rendering. Reusing a source name must keep its
// first snapshot rather than replacing evidence beneath an earlier report.
VALIDATION_TEST(tokenizer_tests, scoped_report) {
  Allocator::Arena render_arena;
  Errors errors;

  {
    Errors::Report empty(
        errors, "empty.ttx"_view, View::Bytes(), Anchor::create(Span()));
  }
  EXPECT(errors.is_empty());

  {
    Allocator::Arena source_arena;
    Tokenizer tokenizer(source_arena, "report source"_view, "report.ttx"_view);
    Token token = tokenizer.get_tokens().get_data()[0];
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        Anchor::create(Span(token)));

    report << "Scoped report "_view << Count(7) << "."_view;
    report.get_hint() << "Use the retained message."_view;
  }

  {
    Allocator::Arena replacement_arena;
    Tokenizer replacement(
        replacement_arena, "replacement source"_view, "report.ttx"_view);
    Errors::Report report(
        errors, replacement.get_source_path(), replacement.get_source_text(),
        Anchor::create(Span(replacement.get_tokens().get_data()[0])));
    report << "Repeated report."_view;
  }

  {
    Errors::Report report(
        errors, "outer.ttx"_view, "outer source"_view, Anchor::create(Span()));
    report << "Outer report."_view;
  }

  View::Bytes scoped = errors.render_message(render_arena, 0);
  View::Bytes repeated = errors.render_message(render_arena, 1);
  View::Bytes outer = errors.render_message(render_arena, 2);

  ASSERT_EQ(errors.get_size(), Count(3));
  EXPECT(Algorithm::search(scoped, "report.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(scoped, "report source"_view) != Count(-1));
  EXPECT(Algorithm::search(scoped, "Scoped report 7."_view) != Count(-1));
  EXPECT(
      Algorithm::search(scoped, "Use the retained message."_view) != Count(-1));
  EXPECT(Algorithm::search(repeated, "report source"_view) != Count(-1));
  EXPECT(Algorithm::search(repeated, "replacement source"_view) == Count(-1));
  EXPECT(Algorithm::search(repeated, "Repeated report."_view) != Count(-1));
  EXPECT(Algorithm::search(outer, "outer.ttx:"_view) != Count(-1));
  EXPECT(Algorithm::search(outer, "Outer report."_view) != Count(-1));
  EXPECT(Algorithm::search(outer, "report.ttx"_view) == Count(-1));
}

// Nested cursors share an error collection while retaining separate source
// contexts. Reports made before and after the inner lifetime must continue
// to render against the correct source.
VALIDATION_TEST(tokenizer_tests, overlapping_cursors) {
  Allocator::Arena render_arena;
  Errors errors;

  {
    Allocator::Arena outer_arena;
    Tokenizer outer_tokenizer(
        outer_arena, "outer start finish"_view, "outer.ttx"_view);
    Cursor outer(outer_tokenizer.get_stream());

    {
      Allocator::Arena inner_arena;
      Tokenizer inner_tokenizer(
          inner_arena, "inner source"_view, "inner.ttx"_view);
      Cursor inner(inner_tokenizer.get_stream());

      errors.create_token_error(
          outer, "Outer while inner is live."_view, "Outer token hint."_view);
      errors.create_token_error(
          inner, "Inner while nested."_view, "Inner token hint."_view);
    }

    Token start = outer.current();
    outer.consume();
    outer.consume();
    Token end = outer.current();
    errors.create_expression_error(
        outer, Span(start, end), "Outer after inner."_view,
        "Outer range hint."_view);
  }

  View::Bytes first = errors.render_message(render_arena, 0);
  View::Bytes second = errors.render_message(render_arena, 1);
  View::Bytes third = errors.render_message(render_arena, 2);

  ASSERT_EQ(errors.get_size(), Count(3));
  EXPECT(Algorithm::search(first, "outer.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(first, "outer start finish"_view) != Count(-1));
  EXPECT(
      Algorithm::search(first, "Outer while inner is live."_view) != Count(-1));
  EXPECT(Algorithm::search(first, "Outer token hint."_view) != Count(-1));
  EXPECT(Algorithm::search(first, "^----\n"_view) != Count(-1));
  EXPECT(Algorithm::search(first, "inner.ttx"_view) == Count(-1));

  EXPECT(Algorithm::search(second, "inner.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "inner source"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "Inner while nested."_view) != Count(-1));
  EXPECT(Algorithm::search(second, "Inner token hint."_view) != Count(-1));
  EXPECT(Algorithm::search(second, "^----\n"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "outer.ttx"_view) == Count(-1));

  EXPECT(Algorithm::search(third, "outer.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(third, "outer start finish"_view) != Count(-1));
  EXPECT(Algorithm::search(third, "Outer after inner."_view) != Count(-1));
  EXPECT(Algorithm::search(third, "Outer range hint."_view) != Count(-1));
  EXPECT(Algorithm::search(third, "^----\n"_view) != Count(-1));
  EXPECT(Algorithm::search(third, "inner.ttx"_view) == Count(-1));
}

// General, token and expression reporting paths must preserve their own
// message and hint. Exercising all three with one Cursor exposes accidental
// reuse of another report context.
VALIDATION_TEST(tokenizer_tests, wrapper_messages) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "first second third"_view, "wrappers.ttx"_view);
  Cursor cursor(tokenizer.get_stream());

  errors.create_error(cursor, "General wrapper."_view, "General hint."_view);
  errors.create_token_error(cursor, "Token wrapper."_view, "Token hint."_view);
  Token start = cursor.current();
  cursor.consume();
  Token end = cursor.current();
  errors.create_expression_error(
      cursor, Span(start, end), "Expression wrapper."_view,
      "Expression hint."_view);

  View::Bytes general = errors.render_message(render_arena, 0);
  View::Bytes token = errors.render_message(render_arena, 1);
  View::Bytes expression = errors.render_message(render_arena, 2);

  ASSERT_EQ(errors.get_size(), Count(3));
  EXPECT(Algorithm::search(general, "wrappers.ttx:"_view) != Count(-1));
  EXPECT(Algorithm::search(general, "General wrapper."_view) != Count(-1));
  EXPECT(Algorithm::search(general, "General hint."_view) != Count(-1));
  EXPECT(Algorithm::search(token, "wrappers.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(token, "Token wrapper."_view) != Count(-1));
  EXPECT(Algorithm::search(token, "Token hint."_view) != Count(-1));
  EXPECT(Algorithm::search(expression, "wrappers.ttx:1:1"_view) != Count(-1));
  EXPECT(
      Algorithm::search(expression, "Expression wrapper."_view) != Count(-1));
  EXPECT(Algorithm::search(expression, "Expression hint."_view) != Count(-1));
}

// A structured report created by Cursor must carry the same source and
// range as its textual helpers. Rendering after the report closes proves
// that the message was published into Errors.
VALIDATION_TEST(tokenizer_tests, cursor_report) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "first second"_view, "cursor-report.ttx"_view);
  Cursor cursor(tokenizer.get_stream());

  Token first = cursor.consume();
  Token second = cursor.current();
  {
    auto report = errors.create_report(cursor, Span(first, second));
    report << "Semantic report."_view;
  }

  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(
      Algorithm::search(rendered, "cursor-report.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "first second"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Semantic report."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "^----\n"_view) != Count(-1));
}

// A reversed expression range must render from the earlier token and retain
// its hint. This checks the diagnostic consumer of Span ordering, including
// the resulting caret location.
VALIDATION_TEST(tokenizer_tests, reversed_range) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "one two three"_view, "reversed.ttx"_view);
  Cursor cursor(tokenizer.get_stream());

  Token early = cursor.current();
  cursor.consume();
  cursor.consume();
  Token late = cursor.current();
  Span span(late, early);
  errors.create_expression_error(
      cursor, span, "Reversed range."_view, "Order the range."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(span.get_start().get_offset() == early.get_offset());
  EXPECT(span.get_end().get_offset() == late.get_offset());
  EXPECT(Algorithm::search(rendered, "reversed.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Reversed range."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Order the range."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "^--\n"_view) != Count(-1));
}

// An operator can be the diagnostic focus inside a broader expression. The
// excerpt must show the whole expression while the caret marks only the
// operator.
VALIDATION_TEST(tokenizer_tests, operator_caret) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "5 == true"_view, "anchor.ttx"_view);
  const Token* tokens = tokenizer.get_tokens().get_data();
  Span expression(tokens[0], tokens[2]);

  {
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        Anchor::create(tokens[1], expression));
    report << "Equal rejects the right operand."_view;
  }

  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "anchor.ttx:1:3"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "5 == true"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "^-\n"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "^--------\n"_view) == Count(-1));
}

// An empty or external focus cannot place a caret inside the retained
// source. The valid surrounding Span still supplies the excerpt and
// fallback location.
VALIDATION_TEST(tokenizer_tests, external_anchor) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "5 + true"_view, "anchor.ttx"_view);
  const Token* tokens = tokenizer.get_tokens().get_data();
  Span expression(tokens[0], tokens[2]);
  Token external(40, 7, 4, 1, Code::Type::AddOp);

  {
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        Anchor::create(Token(), expression));
    report << "Empty focus."_view;
  }

  {
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        Anchor::create(external, expression));
    report << "External focus."_view;
  }

  View::Bytes empty = errors.render_message(render_arena, 0);
  View::Bytes outside = errors.render_message(render_arena, 1);

  ASSERT_EQ(errors.get_size(), Count(2));
  EXPECT(Algorithm::search(empty, "anchor.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(empty, "5 + true"_view) != Count(-1));
  EXPECT(Algorithm::search(empty, "^"_view) == Count(-1));
  EXPECT(Algorithm::search(outside, "anchor.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(outside, "5 + true"_view) != Count(-1));
  EXPECT(Algorithm::search(outside, "^"_view) == Count(-1));
}

// Two source transactions can report under one logical name. Errors must
// continue to use the first retained snapshot after both transaction Arenas
// are gone.
VALIDATION_TEST(tokenizer_tests, retained_source_name) {
  Allocator::Arena render_arena;
  Errors errors;

  {
    Allocator::Arena source_arena;
    Tokenizer tokenizer(source_arena, "first value"_view, "test.ttx"_view);
    Cursor cursor(tokenizer.get_stream());
    errors.create_token_error(cursor, "First failure."_view);
  }

  {
    Allocator::Arena source_arena;
    Tokenizer tokenizer(source_arena, "second value"_view, "test.ttx"_view);
    Cursor cursor(tokenizer.get_stream());
    errors.create_token_error(cursor, "Second failure."_view);
  }

  View::Bytes first = errors.render_message(render_arena, 0);
  View::Bytes second = errors.render_message(render_arena, 1);

  ASSERT_EQ(errors.get_size(), Count(2));
  EXPECT(Algorithm::search(first, "first value"_view) != Count(-1));
  EXPECT(Algorithm::search(first, "second value"_view) == Count(-1));
  EXPECT(Algorithm::search(second, "first value"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "second value"_view) == Count(-1));
}

// A token on a later line must render its own line and column rather than
// the beginning of the source. The multi line fixture also exposes
// incorrect excerpt selection.
VALIDATION_TEST(tokenizer_tests, token_error) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "one\ntwo\nthree value"_view, "test.ttx"_view);
  Token token = tokenizer.get_tokens().get_data()[3];

  {
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        Anchor::create(Span(token)));
    report << "Bad token."_view;
  }

  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "test.ttx:3:7"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Bad token."_view) != Count(-1));
}

// Statement recovery consumes the malformed statement through its boundary.
// The next parser must start on the following authored token.
VALIDATION_TEST(tokenizer_tests, recover_stmt) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "bad tokens ; next"_view, "test.ttx"_view);
  Cursor cursor(tokenizer.get_stream());

  cursor.skip_until({{Code::Type::EndStatement, Code::Type::ScopeEnd}});
  cursor.consume();

  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "next"_view);
}

// Recovery inside a body leaves the closing brace for the enclosing parser.
// Consuming that brace here would make the outer owner close its scope
// twice.
VALIDATION_TEST(tokenizer_tests, recover_scoped_stmt) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "bad tokens } next"_view, "test.ttx"_view);
  Cursor cursor(tokenizer.get_stream());

  cursor.skip_until({{Code::Type::EndStatement, Code::Type::ScopeEnd}});
  cursor.consume_if(Code::Type::EndStatement);

  EXPECT(cursor.matches(Code::Type::ScopeEnd));
}

// A Token carries coordinates rather than a retained text view. Applying
// those coordinates to a second source must project the second source
// bytes.
VALIDATION_TEST(tokenizer_tests, token_projection) {
  Allocator::Arena arena;
  Tokenizer tokenizer(arena, "one two three"_view, "test.ttx"_view);
  Token token = tokenizer.get_tokens().get_data()[1];

  EXPECT_TEXT(token.caculate_text(tokenizer.get_source_text()), "two"_view);
  EXPECT_TEXT(token.caculate_text("red sky green"_view), "sky"_view);
}

// Cursor must accept a classified Stream supplied directly by another
// frontend. The fixture bypasses Tokenizer so traversal cannot silently
// depend on tokenizer private state.
VALIDATION_TEST(tokenizer_tests, cursor_stream) {
  Allocator::Arena arena;
  Errors errors;
  const Static::Vector<Token, 2> tokens = {{
    Token(0, 1, 1, 6, Code::Type::Addressable),
    Token(6, 1, 7, 0, Code::Type::Terminal),
  }};
  struct Frontend {
    View::Vector<Token> tokens;
    auto get_input() const -> tetrodotoxin_source_snapshot {
      return {
        {reinterpret_cast<const U8*>("frontend.ttx"), 12},
        {reinterpret_cast<const U8*>("custom"), 6}};
    }
    auto get_token_buffer() const -> tetrodotoxin_source_tokens {
      return {
        tetrodotoxin_source_token_representation(),
        reinterpret_cast<const U8*>(tokens.get_data()),
        tokens.get_size() * sizeof(Token), tokens.get_size()};
    }
    auto get_lexicon() const -> Perimortem::System::Uuid {
      return Stream::lexical_contract;
    }
    auto get_data() const -> View::Bytes { return {}; }
  } frontend{tokens};
  auto stream = Stream::provide(frontend);
  Cursor cursor(stream);

  EXPECT(cursor.matches(Code::Type::Addressable));
  EXPECT_TEXT(cursor.get_text(), "custom"_view);
  EXPECT_TEXT(cursor.get_source_path(), "frontend.ttx"_view);
}
