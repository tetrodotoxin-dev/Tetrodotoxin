// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/utility/table.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;

using namespace Validation;

using KeywordEntry = Pair<View::Bytes, U8>;
constexpr Static::Vector<KeywordEntry, 23> keyword_source = {{
  KeywordEntry{"as"_view, 1},
  {"if"_view, 2},
  {"for"_view, 3},
  {"new"_view, 4},
  {"else"_view, 5},
  {"func"_view, 6},
  {"init"_view, 7},
  {"self"_view, 8},
  {"true"_view, 9},
  {"alias"_view, 10},
  {"debug"_view, 11},
  {"error"_view, 12},
  {"false"_view, 13},
  {"using"_view, 14},
  {"while"_view, 15},
  {"entity"_view, 16},
  {"object"_view, 17},
  {"return"_view, 18},
  {"struct"_view, 19},
  {"library"_view, 20},
  {"on_load"_view, 21},
  {"package"_view, 22},
  {"warning"_view, 23},
}};

using keyword_table = Table<U32, keyword_source>;

using WordEntry = Pair<View::Bytes, View::Bytes>;
constexpr Static::Vector<WordEntry, 8> word_source = {{
  WordEntry{"a"_view, "b"_view},
  {"b"_view, "test"_view},
  {"longer?"_view, "shorter"_view},
  {"possible?"_view, "maybe!"_view},
  {"happy"_view, "feeling glad"_view},
  {"sunshine"_view, "in a bag"_view},
  {"useless"_view, "not for long"_view},
  {"future"_view, "comming on"_view},
}};

using word_table = Table<View::Bytes, word_source>;

struct Fact {
  View::Bytes name;
  Count byte_size = 0;
  Bool exposed = False;
};

using FactEntry = Pair<View::Bytes, Fact>;
constexpr Static::Vector<FactEntry, 3> fact_source = {{
  FactEntry{"Vec2D"_view, {"Vec2D"_view, 8, True}},
  {"Vec3D"_view, {"Vec3D"_view, 12, True}},
  {"Sampler2D"_view, {"Sampler2D"_view, 0, False}},
}};

using fact_table = Table<Fact, fact_source>;
using fact_table_aligned = Table<Fact, fact_source>;

static Harness StaticTable = {
  .name = "Utility::Table"_view,
};

PERIMORTEM_UNIT_TEST(StaticTable, keyword_table) {
  constexpr auto invalid = -1;
  for (Count i = 0; i < keyword_source.get_size(); i++) {
    EXPECT_EQ(
        keyword_table::find_or_default(keyword_source[i].key, invalid),
        keyword_source[i].value);
  }

  EXPECT_EQ(keyword_table::find_or_default("unknown"_view, invalid), invalid);
  EXPECT_EQ(keyword_table::find_or_default("a"_view, invalid), invalid);
  EXPECT_EQ(keyword_table::find_or_default("As"_view, invalid), invalid);
  EXPECT_EQ(keyword_table::find_or_default(""_view, invalid), invalid);
  EXPECT_EQ(keyword_table::find_or_default("rutern"_view, invalid), invalid);
  EXPECT_EQ(keyword_table::find_or_default("errrr"_view, invalid), invalid);
}

PERIMORTEM_UNIT_TEST(StaticTable, word_table) {
  constexpr auto invalid = "nope"_view;
  for (Count i = 0; i < word_source.get_size(); i++) {
    EXPECT_TEXT(
        word_table::find_or_default(word_source[i].key, invalid),
        word_source[i].value);
  }

  EXPECT_TEXT(word_table::find_or_default("unknown"_view, invalid), invalid);
  EXPECT_TEXT(word_table::find_or_default("c"_view, invalid), invalid);
  EXPECT_TEXT(word_table::find_or_default("As"_view, invalid), invalid);
  EXPECT_TEXT(word_table::find_or_default(""_view, invalid), invalid);
  EXPECT_TEXT(word_table::find_or_default("rutern"_view, invalid), invalid);
  EXPECT_TEXT(word_table::find_or_default("errrr"_view, invalid), invalid);
}

PERIMORTEM_UNIT_TEST(StaticTable, find_or_null) {
  const Fact* vec = fact_table::find_or_null("Vec3D"_view);
  EXPECT(vec != nullptr);
  EXPECT_TEXT(vec->name, "Vec3D"_view);
  EXPECT_EQ(vec->byte_size, 12);
  EXPECT(vec->exposed);

  EXPECT(fact_table::find_or_null("Missing"_view) == nullptr);
  EXPECT(fact_table::find_or_null(""_view) == nullptr);
}
