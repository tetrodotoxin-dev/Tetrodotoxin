// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/memory/dynamic/map.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "unit_tests/perimortem/memory/hashable.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;

using namespace Validation;

static Harness DynamicMap = {
  .name = "Dynamic::Map"_view,
  .setup =
      []() {
        default_construct_count = 0;
        default_destruct_count = 0;
      },
};

PERIMORTEM_UNIT_TEST(DynamicMap, find_and_get_entry) {
  Dynamic::Map<S32, S32> int_map = {{{1, 2}, {2, 3}, {4, 5}}};

  auto found = int_map.find(2);
  ASSERT(found);
  EXPECT_EQ((*found).value, 3);
  EXPECT(!int_map.find(8));

  const auto& const_map = int_map;
  auto const_found = const_map.find(4);
  ASSERT(const_found);
  EXPECT_EQ((*const_found).value, 5);
  EXPECT(!const_map.find(8));

  Count count = 0;
  S32 key_sum = 0;
  S32 value_sum = 0;
  for (Count entry_index = 0; entry_index < int_map.get_size(); entry_index++) {
    auto* entry = int_map.get_entry(entry_index);
    ASSERT(entry != nullptr);
    count++;
    key_sum += entry->key;
    value_sum += entry->value;
  }

  EXPECT_EQ(count, 3);
  EXPECT_EQ(key_sum, 7);
  EXPECT_EQ(value_sum, 10);
  EXPECT(int_map.get_entry(3) == nullptr);
}

PERIMORTEM_UNIT_TEST(DynamicMap, insert_on_index) {
  Dynamic::Map<S32, S32> empty_map;

  // Populate defaults
  for (Count i = 0; i < 10; i++) {
    empty_map[i];
  }

  EXPECT_EQ(empty_map.get_size(), 10);
  for (Count i = 0; i < 10; i++) {
    EXPECT(empty_map.contains(i));
  }
}

PERIMORTEM_UNIT_TEST(DynamicMap, duplicate_keys) {
  Dynamic::Map<S32, S32> int_map = {{{1, 2}, {1, 4}}};

  EXPECT_EQ(int_map.get_size(), 1);
  ASSERT_EQ(int_map[1], 4);

  int_map.insert(2, 8);
  int_map.insert(1, 8);
  ASSERT_EQ(int_map[1], 8);
  EXPECT_EQ(int_map[2], 8);
}

PERIMORTEM_UNIT_TEST(DynamicMap, remove) {
  Dynamic::Map<S32, S32> int_map;

  int_map.ensure_capacity(1000);
  for (Count i = 0; i < 100; i++) {
    int_map.insert(i, i + 2);
  }

  EXPECT(int_map.remove(50));
  EXPECT(!int_map.remove(50));
  EXPECT(!int_map.contains(50));
  EXPECT_EQ(int_map.get_size(), Count(99));
  for (Count i = 0; i < 100; i++) {
    if (i != 50) {
      ASSERT_EQ(int_map[i], i + 2);
    }
  }
}

PERIMORTEM_UNIT_TEST(DynamicMap, empty_keys) {
  Dynamic::Map<Dynamic::Bytes, S32> empty_map;

  Count i = 0;
  while (i < 1000) {
    i++;
    empty_map.insert(Dynamic::Bytes(), i);
  }

  // Map should contain a single key which maps to the last value used.
  EXPECT_EQ(empty_map.get_size(), 1);
  EXPECT_EQ(empty_map[Dynamic::Bytes()], i);
}

PERIMORTEM_UNIT_TEST(DynamicMap, insert_stress_test) {
  Dynamic::Map<S32, S32> large_map;
  for (Count i = 0; i < 1000; i++) {
    large_map.insert(i, i + 2);
  }

  EXPECT_EQ(large_map.get_size(), 1000);
  for (Count i = 0; i < 1000; i++) {
    ASSERT_EQ(large_map[i], i + 2);
  }
}

PERIMORTEM_UNIT_TEST(DynamicMap, key_construction) {
  Count construct_count = 0;
  Count destruct_count = 0;

  {
    Dynamic::Map<Hashable, S32> custom_map;
    for (Count i = 0; i < 100; i++) {
      custom_map.insert(Hashable(i, construct_count, destruct_count), i);
    }

    EXPECT_EQ(custom_map.get_size(), 100);
    for (Count i = 0; i < 100; i++) {
      ASSERT_EQ(custom_map[Hashable(i, construct_count, destruct_count)], i);
    }
  }

  EXPECT_EQ(construct_count, destruct_count);
  EXPECT_EQ(default_construct_count, default_destruct_count);
}

PERIMORTEM_UNIT_TEST(DynamicMap, value_construction) {
  Count construct_count = 0;
  Count destruct_count = 0;

  {
    Dynamic::Map<S32, Hashable> custom_map;
    for (Count i = 0; i < 100; i++) {
      custom_map.insert(i, Hashable(i, construct_count, destruct_count));
    }

    EXPECT_EQ(custom_map.get_size(), 100);
    for (Count i = 0; i < 100; i++) {
      ASSERT(custom_map[i] == Hashable(i, construct_count, destruct_count));
    }
  }

  EXPECT_EQ(construct_count, destruct_count);
  EXPECT_EQ(default_construct_count, default_destruct_count);
}

PERIMORTEM_UNIT_TEST(DynamicMap, dynamic_keys) {
  Dynamic::Map<Dynamic::Bytes, S32> text_map;

  text_map["Hello"_view] = 0;
  text_map["World"_view] = 1;

  // Byte keys
  Dynamic::Bytes text;
  text.append('a');
  for (U8 ch = 'A'; ch < 'z'; ch++) {
    text.get_access().get_data()[0] = ch;
    text_map[text] = 2 + ch;
  }

  text_map["Longer test string"_view] = 2;

  ASSERT_EQ(text_map["Hello"_view], 0);
  ASSERT_EQ(text_map["World"_view], 1);
  ASSERT_EQ(text_map["Longer test string"_view], 2);
  for (U8 ch = 'A'; ch < 'z'; ch++) {
    text.get_access().get_data()[0] = ch;
    ASSERT_EQ(text_map[text], 2 + ch);
  }

  ASSERT_EQ(text_map["Longer test string"_view], 2);

  auto copied = text_map;
  ASSERT_EQ(copied["Hello"_view], 0);
  ASSERT_EQ(copied["Longer test string"_view], 2);
}

PERIMORTEM_UNIT_TEST(DynamicMap, dynamic_value) {
  Dynamic::Map<S32, Dynamic::Bytes> text_map;

  text_map[0] = "Hello"_view;
  text_map[1] = "World"_view;
  text_map[2] = "Longer test string"_view;

  ASSERT_TEXT(text_map[0].get_view(), "Hello"_view);
  ASSERT_TEXT(text_map[1].get_view(), "World"_view);
  ASSERT_TEXT(text_map[2].get_view(), "Longer test string"_view);

  auto copied = text_map;
  ASSERT_TEXT(copied[0].get_view(), "Hello"_view);
  ASSERT_TEXT(copied[2].get_view(), "Longer test string"_view);
}

PERIMORTEM_UNIT_TEST(DynamicMap, assignment) {
  Dynamic::Map<S32, S32> source = {{{1, 2}, {3, 4}}};
  Dynamic::Map<S32, S32> destination = {{{5, 6}}};

  destination = source;
  source[1] = 8;

  EXPECT_EQ(destination.get_size(), Count(2));
  EXPECT_EQ(destination[1], 2);
  EXPECT_EQ(destination[3], 4);
}

PERIMORTEM_UNIT_TEST(DynamicMap, move_assignment) {
  Dynamic::Map<S32, S32> source = {{{1, 2}, {3, 4}}};
  Dynamic::Map<S32, S32> destination = {{{5, 6}}};

  destination = static_cast<Dynamic::Map<S32, S32>&&>(source);

  EXPECT_EQ(destination.get_size(), Count(2));
  EXPECT_EQ(destination[1], 2);
}

PERIMORTEM_UNIT_TEST(DynamicMap, reuse) {
  Dynamic::Map<S32, S32> reuse_map;
  for (S32 loops = 0; loops < 5; loops++) {
    reuse_map.clear();
    ASSERT_EQ(reuse_map.get_size(), 0);
    for (Count i = 0; i < 100; i++) {
      reuse_map[i] = i;
    }

    ASSERT_EQ(reuse_map.get_size(), 100);
    for (Count i = 0; i < 100; i++) {
      ASSERT_EQ(reuse_map[i], i);
    }
  }
}

PERIMORTEM_UNIT_TEST(DynamicMap, leak_test) {
  auto pre_test_memory = Bibliotheca::allocated_memory();

  {
    Dynamic::Map<Dynamic::Bytes, Dynamic::Bytes> memory_intensive;
    Dynamic::Bytes source;
    for (Count i = 0; i < 100; i++) {
      source.append('A');
      memory_intensive[source] = "Test text to copy"_view;
    }

    ASSERT_EQ(memory_intensive.get_size(), 100);
    EXPECT(memory_intensive.remove(source));
    ASSERT_EQ(memory_intensive.get_size(), 99);
  }

  {
    Dynamic::Map<S32, S32> large_map;
    for (Count i = 0; i < 1000; i++) {
      large_map.insert(i, i + 2);
    }
  }

  auto post_test_memory = Bibliotheca::allocated_memory();
  EXPECT_EQ(pre_test_memory, post_test_memory);
}
