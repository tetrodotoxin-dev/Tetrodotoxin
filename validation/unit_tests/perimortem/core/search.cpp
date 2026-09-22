// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/algorithm/search.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness AlgoSearch = {
  .name = "Core::Algorithm::Search"_view,
};

template <Count size>
auto populate_test(View::Bytes text, Count location) -> Static::Bytes<size> {
  Static::Bytes<size> test_data;
  // Create junk data to make sure filtering works.
  for (Count i = 0; i < test_data.get_size(); i++) {
    test_data[i] = U8(i);
  }

  Data::copy(test_data.get_data() + location, text.get_data(), text.get_size());
  return test_data;
}

template <Count size>
auto load_text(Static::Bytes<size>& test_data, View::Bytes text, Count location)
    -> void {
  Data::copy(test_data.get_data() + location, text.get_data(), text.get_size());
}

PERIMORTEM_UNIT_TEST(AlgoSearch, boundaries) {
  EXPECT_EQ(Algorithm::search(""_view, ""_view), Count(0));
  EXPECT_EQ(Algorithm::search(""_view, "abc"_view), Count(-1));
  EXPECT_EQ(Algorithm::search("a"_view, "a"_view), Count(0));
  EXPECT_EQ(Algorithm::search("a"_view, "b"_view), Count(-1));
}

PERIMORTEM_UNIT_TEST(AlgoSearch, byte_offset) {
  Static::Bytes<160> source;
  for (Count i = 0; i < source.get_size(); i++) {
    source[i] = 'a';
  }

  source[97] = '\\';

  EXPECT_EQ(Algorithm::search(source, U8('\\')), Count(97));
  EXPECT_EQ(Algorithm::search(source, U8('z')), Count(-1));
}

PERIMORTEM_UNIT_TEST(AlgoSearch, find) {
  constexpr auto test_word = "world"_view;
  EXPECT_EQ(
      Algorithm::search(populate_test<5>(test_word, 0), test_word), Count(0));
  EXPECT_EQ(
      Algorithm::search(populate_test<70>(test_word, 0), test_word), Count(0));
  EXPECT_EQ(
      Algorithm::search(populate_test<70>(test_word, 40), test_word),
      Count(40));
  EXPECT_EQ(
      Algorithm::search(populate_test<70>(test_word, 65), test_word),
      Count(65));

  // Create a sparsely filled array with junk data and a few near misses.
  constexpr Count location = 982;
  auto test_text = populate_test<1240>(test_word, location);
  constexpr auto almost_word = "woold"_view;
  load_text(test_text, almost_word, 120);
  load_text(test_text, almost_word, 723);
  load_text(test_text, almost_word, 600);
  load_text(test_text, almost_word, 620);
  load_text(test_text, almost_word, 12);
  load_text(test_text, almost_word, 800);
  EXPECT_EQ(Algorithm::search(test_text, test_word), location);

  // Write over the only correct instance.
  load_text(test_text, almost_word, location);
  EXPECT_EQ(Algorithm::search(test_text, test_word), Count(-1));
}

PERIMORTEM_UNIT_TEST(AlgoSearch, multiple_matches) {
  constexpr auto test_word = "perimortem testing"_view;
  constexpr Static::Vector<Count, 12> word_locations = {{
    12,
    120,
    245,
    381,
    422,
    589,
    690,
    690 + test_word.get_size(),
    690 + test_word.get_size() * 2,
    690 + test_word.get_size() * 3,
    810,
    830,
  }};

  Static::Bytes<
      word_locations[word_locations.get_size() - 1] + test_word.get_size() + 1>
      test_text;
  for (Count i = 0; i < word_locations.get_size(); i++) {
    load_text(test_text, test_word, word_locations[i]);
  }

  for (Count i = 0; i < word_locations.get_size(); i++) {
    EXPECT_EQ(Algorithm::search(test_text, test_word), word_locations[i]);
    load_text(test_text, "deleted"_view, word_locations[i]);
  }
}

PERIMORTEM_UNIT_TEST(AlgoSearch, very_large_key) {
  constexpr auto test_word = "perimortem testing on very large strings"_view;
  constexpr Static::Vector<Count, 11> word_locations = {{
    12,
    120,
    245,
    381,
    422,
    589,
    690,
    690 + test_word.get_size(),
    690 + test_word.get_size() * 2,
    690 + test_word.get_size() * 3,
    930,
  }};

  Static::Bytes<
      word_locations[word_locations.get_size() - 1] + test_word.get_size() + 1>
      test_text;
  for (Count i = 0; i < word_locations.get_size(); i++) {
    load_text(test_text, test_word, word_locations[i]);
  }

  for (Count i = 0; i < word_locations.get_size(); i++) {
    EXPECT_EQ(Algorithm::search(test_text, test_word), word_locations[i]);
    load_text(test_text, "deleted"_view, word_locations[i]);
  }
}
