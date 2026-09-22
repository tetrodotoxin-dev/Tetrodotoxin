// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/memory/dynamic/set.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/hash.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "unit_tests/perimortem/memory/hashable.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Core;
using namespace Validation;

static Harness DynamicSet = {
  .name = "Dynamic::Set"_view,
  .setup =
      []() {
        default_construct_count = 0;
        default_destruct_count = 0;
      },
};

PERIMORTEM_UNIT_TEST(DynamicSet, unique_values) {
  Dynamic::Set<S32> values;

  EXPECT(values.insert(1));
  EXPECT(!values.insert(1));
  EXPECT(values.insert(4));
  EXPECT(values.contains(1));
  EXPECT(values.contains(4));
  EXPECT(!values.contains(2));
  EXPECT_EQ(values.get_size(), 2);

  values.clear();
  EXPECT(values.is_empty());
  EXPECT(!values.contains(1));
}

PERIMORTEM_UNIT_TEST(DynamicSet, find) {
  Dynamic::Set<S32> values(4);

  values.insert(1);
  values.insert(2);
  values.insert(4);

  auto found = values.find(2);
  ASSERT(found);
  EXPECT_EQ(*found, 2);
  EXPECT(!values.find(8));

  const auto& const_values = values;
  auto const_found = const_values.find(4);
  ASSERT(const_found);
  EXPECT_EQ(*const_found, 4);
  EXPECT(!const_values.find(8));
}

PERIMORTEM_UNIT_TEST(DynamicSet, visit) {
  Dynamic::Set<S32> values;
  values.insert(1);
  values.insert(2);
  values.insert(4);

  S32 total = 0;
  values.visit([&total](S32& value) { total += value; });
  EXPECT_EQ(total, 7);

  total = 0;
  const Dynamic::Set<S32>& const_values = values;
  const_values.visit([&total](const S32& value) { total += value; });
  EXPECT_EQ(total, 7);
}

PERIMORTEM_UNIT_TEST(DynamicSet, remove) {
  Dynamic::Set<S32> values;

  values.insert(1);
  values.insert(2);
  values.insert(4);

  EXPECT(values.remove(2));
  EXPECT(!values.remove(2));
  EXPECT_EQ(values.get_size(), 2);
  EXPECT(values.contains(1));
  EXPECT(!values.contains(2));
  EXPECT(values.contains(4));

  EXPECT(values.insert(2));
  EXPECT(values.contains(2));
}

PERIMORTEM_UNIT_TEST(DynamicSet, remove_collisions) {
  class CollisionKey {
   public:
    CollisionKey(Count value) : value(value) {}

    constexpr auto hash() const -> U64 { return 7; }
    constexpr auto operator==(const CollisionKey& rhs) const -> Bool {
      return value == rhs.value;
    }

   private:
    Count value = 0;
  };

  Dynamic::Set<CollisionKey> values;
  for (Count i = 0; i < 24; i++) {
    EXPECT(values.insert(CollisionKey(i)));
  }

  EXPECT(values.remove(CollisionKey(8)));
  EXPECT(!values.contains(CollisionKey(8)));
  for (Count i = 0; i < 24; i++) {
    if (i == 8) {
      continue;
    }

    ASSERT(values.contains(CollisionKey(i)));
  }

  EXPECT(values.insert(CollisionKey(8)));
  EXPECT(values.contains(CollisionKey(8)));
}

PERIMORTEM_UNIT_TEST(DynamicSet, remove_displacements) {
  class ProbeKey {
   public:
    ProbeKey(Count home, Count id) : home(home), id(id) {}

    // Tests can build exact probe chains because Core::Hash delegates to this
    // hook for custom key types.
    constexpr auto hash() const -> U64 { return home; }
    constexpr auto operator==(const ProbeKey& rhs) const -> Bool {
      return home == rhs.home && id == rhs.id;
    }

   private:
    Count home = 0;
    Count id = 0;
  };

  EXPECT_EQ(Hash(ProbeKey(3, 99)).get_value(), 3);

  {
    Dynamic::Set<ProbeKey> values(8);

    EXPECT(values.insert(ProbeKey(0, 0)));
    EXPECT(values.insert(ProbeKey(0, 1)));
    EXPECT(values.insert(ProbeKey(1, 2)));

    EXPECT(values.remove(ProbeKey(0, 0)));
    EXPECT(!values.contains(ProbeKey(0, 0)));
    EXPECT(values.contains(ProbeKey(0, 1)));
    EXPECT(values.contains(ProbeKey(1, 2)));
  }

  {
    Dynamic::Set<ProbeKey> values(8);

    EXPECT(values.insert(ProbeKey(0, 0)));
    EXPECT(values.insert(ProbeKey(1, 1)));
    EXPECT(values.insert(ProbeKey(2, 2)));
    EXPECT(values.insert(ProbeKey(3, 3)));
    EXPECT(values.insert(ProbeKey(0, 4)));

    EXPECT(values.remove(ProbeKey(0, 0)));
    EXPECT(!values.contains(ProbeKey(0, 0)));
    EXPECT(values.contains(ProbeKey(1, 1)));
    EXPECT(values.contains(ProbeKey(2, 2)));
    EXPECT(values.contains(ProbeKey(3, 3)));
    EXPECT(values.contains(ProbeKey(0, 4)));
  }
}

PERIMORTEM_UNIT_TEST(DynamicSet, insert_stress_test) {
  Dynamic::Set<S32> values;
  for (Count i = 0; i < 1000; i++) {
    EXPECT(values.insert(i));
  }

  EXPECT_EQ(values.get_size(), 1000);
  for (Count i = 0; i < 1000; i++) {
    ASSERT(values.contains(i));
  }
}

PERIMORTEM_UNIT_TEST(DynamicSet, dynamic_keys) {
  Dynamic::Set<Dynamic::Bytes> values;

  EXPECT(values.insert("Hello"_view));
  EXPECT(values.insert("World"_view));
  EXPECT(!values.insert("Hello"_view));
  EXPECT(values.contains("Hello"_view));
  EXPECT(values.contains("World"_view));
  EXPECT(!values.contains("Missing"_view));
  EXPECT_EQ(values.get_size(), 2);
}

PERIMORTEM_UNIT_TEST(DynamicSet, remove_dynamic) {
  Dynamic::Set<Dynamic::Bytes> values;

  values.insert("Hello"_view);
  values.insert("World"_view);
  values.insert("Longer test string"_view);

  EXPECT(values.remove("World"_view));
  EXPECT(values.contains("Hello"_view));
  EXPECT(!values.contains("World"_view));
  EXPECT(values.contains("Longer test string"_view));
  EXPECT_EQ(values.get_size(), 2);
}

PERIMORTEM_UNIT_TEST(DynamicSet, pointer_keys) {
  class StableObject {
   public:
    StableObject(Count id) : id(id) {}

    constexpr auto get_id() const -> Count { return id; }

   private:
    Count id = 0;
  };

  class StableObjectKey {
   public:
    StableObjectKey(const StableObject* object) : object(object) {}

    auto hash() const -> U64 {
      return Hash(U64(reinterpret_cast<CppSize>(object))).get_value();
    }

    constexpr auto operator==(const StableObjectKey& rhs) const -> Bool {
      return object == rhs.object;
    }

    constexpr auto get_object() const -> const StableObject* { return object; }

   private:
    const StableObject* object = nullptr;
  };

  StableObject first(1);
  StableObject second(2);
  StableObject third(3);

  Dynamic::Set<StableObjectKey> values;
  EXPECT(values.insert(&first));
  EXPECT(values.insert(&second));
  EXPECT(values.insert(&third));
  EXPECT(!values.insert(&second));

  auto found = values.find(&second);
  ASSERT(found);
  EXPECT((*found).get_object() == &second);
  EXPECT_EQ((*found).get_object()->get_id(), 2);

  EXPECT(values.remove(&second));
  EXPECT(!values.contains(&second));
  EXPECT(values.contains(&first));
  EXPECT(values.contains(&third));
}

PERIMORTEM_UNIT_TEST(DynamicSet, key_construction) {
  Count construct_count = 0;
  Count destruct_count = 0;

  {
    Dynamic::Set<Hashable> values;
    for (Count i = 0; i < 100; i++) {
      values.insert(Hashable(i, construct_count, destruct_count));
    }

    EXPECT_EQ(values.get_size(), 100);
    for (Count i = 0; i < 100; i++) {
      ASSERT(values.contains(Hashable(i, construct_count, destruct_count)));
    }
  }

  EXPECT_EQ(construct_count, destruct_count);
  EXPECT_EQ(default_construct_count, default_destruct_count);
}

PERIMORTEM_UNIT_TEST(DynamicSet, reuse) {
  Dynamic::Set<S32> values;
  for (S32 loops = 0; loops < 5; loops++) {
    values.clear();
    ASSERT_EQ(values.get_size(), 0);
    for (Count i = 0; i < 100; i++) {
      values.insert(i);
    }

    ASSERT_EQ(values.get_size(), 100);
    for (Count i = 0; i < 100; i++) {
      ASSERT(values.contains(i));
    }
  }
}

PERIMORTEM_UNIT_TEST(DynamicSet, leak_test) {
  auto pre_test_memory = Bibliotheca::allocated_memory();

  {
    Dynamic::Set<Dynamic::Bytes> memory_intensive;
    Dynamic::Bytes source;
    for (Count i = 0; i < 100; i++) {
      source.append('A');
      memory_intensive.insert(source);
      if (i % 2 == 0) {
        memory_intensive.remove(source);
      }
    }

    ASSERT_EQ(memory_intensive.get_size(), 50);
  }

  {
    Dynamic::Set<S32> large_set;
    for (Count i = 0; i < 1000; i++) {
      large_set.insert(i);
    }
  }

  {
    Dynamic::Set<Dynamic::Bytes> first;
    Dynamic::Set<Dynamic::Bytes> second;

    first.insert("old block"_view);
    second.insert("new block"_view);
    first = static_cast<Dynamic::Set<Dynamic::Bytes>&&>(second);
    ASSERT(first.contains("new block"_view));
    ASSERT(!first.contains("old block"_view));
  }

  auto post_test_memory = Bibliotheca::allocated_memory();
  EXPECT_EQ(pre_test_memory, post_test_memory);
}
