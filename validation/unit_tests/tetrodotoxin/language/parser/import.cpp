// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/parser/import.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/source/lexical/associations.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness ImportParser = {
  .name = "Tetrodotoxin::Language::Parser::Import"_view,
};

PERIMORTEM_UNIT_TEST(ImportParser, alias_qualifier) {
  static constexpr Static::Vector<View::Bytes, 4> accepted = {{
    "public Local : alias = source(\"./local.ttx\");"_view,
    "public Math : alias = package(.name = \"Perimortem.Math\", .version = \"1.0\");"_view,
    "private Pixel : alias = source(\"pixel.ttx\")::Pixel;"_view,
    "public Value : alias = package(.name = \"Example.Api\", .version = \"2.1\")::Public::Value;"_view,
  }};

  for (Count i = 0; i < accepted.get_size(); i++) {
    Allocator::Arena arena;
    Errors errors;
    Tokenizer tokenizer(arena, accepted[i], "import.ttx"_view);
    Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations, "import.ttx"_view);

    ASSERT(Language::Parser::Import::is_next(cursor));
    auto imported =
        Language::Parser::Import::parse(cursor, Tetrodotoxin::Source::Documentation::get_empty());
    ASSERT(imported);
    if (i == 2) {
      EXPECT(imported->get_visibility() == Language::Visibility::Private);
      EXPECT_TEXT(imported->get_route(), "Pixel"_view);
    } else if (i == 3) {
      EXPECT_TEXT(imported->get_route(), "Public::Value"_view);
    }
    EXPECT(cursor.matches(Code::Type::Terminal));
    EXPECT(errors.is_empty());
  }

  static constexpr View::Bytes ordinary = "public Local : alias = Other;"_view;
  Allocator::Arena ordinary_arena;
  Errors ordinary_errors;
  Tokenizer ordinary_tokenizer(
      ordinary_arena, ordinary, "ordinary-alias.ttx"_view);
  Associations ordinary_associations(ordinary_tokenizer.get_arena());
  Cursor ordinary_cursor(
      ordinary_tokenizer, ordinary_errors, ordinary_associations,
      "ordinary-alias.ttx"_view);
  EXPECT_NOT(Language::Parser::Import::is_next(ordinary_cursor));
  EXPECT(ordinary_errors.is_empty());

  static constexpr View::Bytes uppercase =
      "public Local : Alias = source(\"local.ttx\");"_view;
  Allocator::Arena uppercase_arena;
  Errors uppercase_errors;
  Tokenizer uppercase_tokenizer(
      uppercase_arena, uppercase, "uppercase-alias.ttx"_view);
  Associations uppercase_associations(uppercase_tokenizer.get_arena());
  Cursor uppercase_cursor(
      uppercase_tokenizer, uppercase_errors, uppercase_associations,
      "uppercase-alias.ttx"_view);
  EXPECT_NOT(Language::Parser::Import::is_next(uppercase_cursor));
  EXPECT_NOT(
      Language::Parser::Import::parse(
          uppercase_cursor, Tetrodotoxin::Source::Documentation::get_empty()));
  EXPECT_NOT(uppercase_errors.is_empty());
}
