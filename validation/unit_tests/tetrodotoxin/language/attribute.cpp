// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/attribute.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Language;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness AttributeTests = {
  .name = "Tetrodotoxin::Language::Attribute"_view,
};

PERIMORTEM_UNIT_TEST(AttributeTests, optional_prefix) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "Value"_view, "<optional attribute>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  auto attributes = Attribute::parse(cursor);

  EXPECT(attributes.is_empty());
  EXPECT(cursor.matches(Code::Type::Type));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AttributeTests, scalar_prefix) {
  static constexpr View::Bytes source =
      "@marker @text(\"value\") @unsigned(7) @signed(-2) "
      "@real(-0.5) @enabled(true) @hex(0x2a) @negative_hex(-0x2a) Value"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "<attribute values>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  auto attributes = Attribute::parse(cursor);

  ASSERT(attributes.get_size() == 8);
  const auto* data = attributes.get_data();
  EXPECT_TEXT(data[0].get_key(), "marker"_view);
  EXPECT(!data[0].has_value());
  EXPECT_TEXT(*data[1].get_value().find<View::Bytes>(), "value"_view);
  EXPECT_EQ(*data[2].get_value().find<U64>(), U64(7));
  EXPECT_EQ(*data[3].get_value().find<S64>(), S64(-2));
  EXPECT_EQ(*data[4].get_value().find<R64>(), R64(-0.5));
  EXPECT_EQ(*data[5].get_value().find<Bool>(), True);
  EXPECT_EQ(*data[6].get_value().find<U64>(), U64(42));
  EXPECT_EQ(*data[7].get_value().find<S64>(), S64(-42));
  EXPECT_TEXT(
      data[1].get_anchor().get_token().caculate_text(source), "text"_view);
  EXPECT_TEXT(
      data[1].get_anchor().get_span().caculate_text(source),
      "text(\"value\")"_view);
  EXPECT(cursor.matches(Code::Type::Type));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AttributeTests, malformed_inputs) {
  static constexpr View::Bytes sources[] = {
    "@valid @invalid() Value"_view,
    "@"_view,
    "@ Value"_view,
    "@text(\"unterminated)"_view,
  };

  for (View::Bytes source : sources) {
    Allocator::Arena arena;
    Errors errors;
    Tokenizer tokenizer(arena, source, "<invalid attribute>"_view);
    Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);

    auto attributes = Attribute::parse(cursor);

    EXPECT(attributes.is_empty());
    EXPECT(!errors.is_empty());
  }
}
