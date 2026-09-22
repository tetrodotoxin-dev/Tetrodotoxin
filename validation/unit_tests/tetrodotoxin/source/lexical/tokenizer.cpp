// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/lexical/tokenizer.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/source/lexical/lexicon.hpp"
#include "tetrodotoxin/source/lexical/span.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness TtxLexical = {
  .name = "TTX::Lexical"_view,
};

PERIMORTEM_UNIT_TEST(TtxLexical, access_operators) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "Perimortem.Graphics color.[r, g] color:[start, 2] "
      "layout[Type] value.member"_view,
      "Test.Package"_view);

  const auto& tokens = tokenizer;
  const auto& token_data = tokenizer;
  ASSERT_EQ(tokens.get_size(), Count(23));

  EXPECT(token_data[0].get_code() == Code::Type::Type);
  EXPECT(token_data[1].get_code() == Code::Type::AddressOp);
  EXPECT_TEXT(tokenizer.get_text(token_data[1]), "."_view);
  EXPECT(token_data[2].get_code() == Code::Type::Type);

  EXPECT_TEXT(tokenizer.get_text(token_data[4]), ".["_view);
  EXPECT(token_data[4].get_code() == Code::Type::SwizzleOp);
  EXPECT_TEXT(tokenizer.get_text(token_data[10]), ":["_view);
  EXPECT(token_data[10].get_code() == Code::Type::ValueAccessOp);

  EXPECT_TEXT(tokenizer.get_text(token_data[16]), "["_view);
  EXPECT(token_data[16].get_code() == Code::Type::BracketStart);
  EXPECT_TEXT(tokenizer.get_text(token_data[20]), "."_view);
  EXPECT(token_data[20].get_code() == Code::Type::AddressOp);
}

PERIMORTEM_UNIT_TEST(TtxLexical, division_tokens) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "/> // retained comment\nleft / right -> call value > other >= floor"_view,
      "Test.Package"_view);

  const auto& tokens = tokenizer;
  const auto& token_data = tokenizer;
  ASSERT_EQ(tokens.get_size(), Count(14));

  EXPECT(token_data[0].get_code() == Code::Type::DivOp);
  EXPECT_TEXT(tokenizer.get_text(token_data[0]), "/"_view);
  EXPECT(token_data[1].get_code() == Code::Type::GreaterOp);
  EXPECT_TEXT(tokenizer.get_text(token_data[1]), ">"_view);
  EXPECT(token_data[2].get_code() == Code::Type::Comment);
  EXPECT(token_data[4].get_code() == Code::Type::DivOp);
  EXPECT(token_data[6].get_code() == Code::Type::CallOp);
  EXPECT(token_data[9].get_code() == Code::Type::GreaterOp);
  EXPECT(token_data[11].get_code() == Code::Type::GreaterEqOp);
  EXPECT(token_data[13].get_code() == Code::Type::Terminal);
}

PERIMORTEM_UNIT_TEST(TtxLexical, raw_comments) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena, "/// Tetrodotoxin\n// Public documentation.\n//// metadata"_view,
      "Test.Package"_view);

  const auto& tokens = tokenizer;
  ASSERT_EQ(tokens.get_size(), Count(4));
  EXPECT(tokens[0].get_code() == Code::Type::RawComment);
  EXPECT_TEXT(tokenizer.get_text(tokens[0]), "/// Tetrodotoxin"_view);
  EXPECT(tokens[1].get_code() == Code::Type::Comment);
  EXPECT_TEXT(tokenizer.get_text(tokens[1]), "// Public documentation."_view);
  EXPECT(tokens[2].get_code() == Code::Type::RawComment);
  EXPECT_TEXT(tokenizer.get_text(tokens[2]), "//// metadata"_view);
  EXPECT(tokens[3].get_code() == Code::Type::Terminal);
}

PERIMORTEM_UNIT_TEST(TtxLexical, propagation_operator) {
  Allocator::Arena arena;
  Tokenizer tokenizer(arena, "value?\nnext?"_view, "Test.Package"_view);

  const auto& tokens = tokenizer;
  const auto& token_data = tokenizer;
  ASSERT_EQ(tokens.get_size(), Count(5));
  EXPECT(token_data[0].get_code() == Code::Type::Addressable);
  EXPECT(token_data[1].get_code() == Code::Type::QuestionOp);
  EXPECT_TEXT(tokenizer.get_text(token_data[1]), "?"_view);
  EXPECT_EQ(tokenizer.get_extent(token_data[1]).get_offset(), U32(5));
  EXPECT_EQ(tokenizer.get_extent(token_data[1]).get_size(), U32(1));
  EXPECT(token_data[2].get_code() == Code::Type::Addressable);
  EXPECT(token_data[3].get_code() == Code::Type::QuestionOp);
  EXPECT_TEXT(tokenizer.get_text(token_data[3]), "?"_view);
  EXPECT_EQ(tokenizer.get_extent(token_data[3]).get_offset(), U32(11));
  EXPECT_EQ(tokenizer.get_extent(token_data[3]).get_size(), U32(1));
  EXPECT(token_data[4].get_code() == Code::Type::Terminal);
}

PERIMORTEM_UNIT_TEST(TtxLexical, reserved_keywords) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "public private expose state const enum struct object using new package "
      "emit emitter @package_name @public"_view,
      "Test.Package"_view);

  const auto& tokens = tokenizer;
  const auto& token_data = tokenizer;
  static constexpr Code::Type expected[] = {
    Code::Type::Public,      Code::Type::Private, Code::Type::Expose,
    Code::Type::State,       Code::Type::Const,   Code::Type::Enum,
    Code::Type::Struct,      Code::Type::Object,  Code::Type::Using,
    Code::Type::New,         Code::Type::Package, Code::Type::Emit,
    Code::Type::Addressable,
  };
  static constexpr Count expected_size = sizeof(expected) / sizeof(*expected);
  ASSERT_EQ(tokens.get_size(), expected_size + 3);
  for (Count i = 0; i < expected_size; i++) {
    EXPECT(token_data[i].get_code() == expected[i]);
  }

  EXPECT(token_data[expected_size].get_code() == Code::Type::Attribute);
  EXPECT_TEXT(
      tokenizer.get_text(token_data[expected_size]), "package_name"_view);
  EXPECT(token_data[expected_size + 1].get_code() == Code::Type::Attribute);
  EXPECT_TEXT(tokenizer.get_text(token_data[expected_size + 1]), "public"_view);
  EXPECT(token_data[expected_size + 2].get_code() == Code::Type::Terminal);
}

PERIMORTEM_UNIT_TEST(TtxLexical, attribute_spelling) {
  Allocator::Arena arena;
  Tokenizer tokenizer(arena, "@first @second value"_view, "Test.Package"_view);

  const auto& tokens = tokenizer;
  ASSERT_EQ(tokens.get_size(), Count(4));

  EXPECT(tokens[0].get_code() == Code::Type::Attribute);
  EXPECT_TEXT(tokenizer.get_text(tokens[0]), "first"_view);
  EXPECT_EQ(tokenizer.get_extent(tokens[0]).get_offset(), U32(1));
  EXPECT_EQ(tokenizer.get_extent(tokens[0]).get_size(), U32(5));

  EXPECT(tokens[1].get_code() == Code::Type::Attribute);
  EXPECT_TEXT(tokenizer.get_text(tokens[1]), "second"_view);
  EXPECT_EQ(tokenizer.get_extent(tokens[1]).get_offset(), U32(8));
  EXPECT_EQ(tokenizer.get_extent(tokens[1]).get_size(), U32(6));

  EXPECT(tokens[2].get_code() == Code::Type::Addressable);
  EXPECT_TEXT(tokenizer.get_text(tokens[2]), "value"_view);
  EXPECT_EQ(tokenizer.get_extent(tokens[2]).get_offset(), U32(15));
  EXPECT_EQ(tokenizer.get_extent(tokens[2]).get_size(), U32(5));

  EXPECT(tokens[3].get_code() == Code::Type::Terminal);
  EXPECT_EQ(tokenizer.get_extent(tokens[3]).get_offset(), U32(20));
}

PERIMORTEM_UNIT_TEST(TtxLexical, lexicon) {
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

PERIMORTEM_UNIT_TEST(TtxLexical, spelling_validation) {
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

PERIMORTEM_UNIT_TEST(TtxLexical, code_semantics) {
  using Code = Code;

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

PERIMORTEM_UNIT_TEST(TtxLexical, hexadecimal_code) {
  Allocator::Arena arena;
  Tokenizer tokenizer(arena, "0xAB"_view, "test.ttx"_view);

  const auto& tokens = tokenizer;
  ASSERT_EQ(tokens.get_size(), Count(2));
  EXPECT(tokens[0].get_code() == Code::Type::Hex);
  EXPECT_TEXT(tokenizer.get_text(tokens[0]), "0xAB"_view);
  EXPECT(tokens[1].get_code() == Code::Type::Terminal);
}
