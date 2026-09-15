// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/memory/dynamic/vector.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness DynamicVector = {
  .name = "Dynamic::Vector"_view,
};

template <typename type>
static constexpr bool supports_forgetful_resize =
    requires(Dynamic::Vector<type>& values) { values.forgetful_resize(1); };

// Ordinary resize can support owning values, while forgetful resize skips
// their lifecycle entirely. Make sure the API rejects types that aren't
// trivially constructable and destructable, including a deleted constructor,
// without excluding the whole Vector type.
class ConstructedValue {
 public:
  ConstructedValue() : value(42) {}
  U32 value;
};

class DestructedValue {
 public:
  DestructedValue() = default;
  ~DestructedValue() {}
};

class UnconstructibleValue {
 public:
  UnconstructibleValue() = delete;
};

static_assert(supports_forgetful_resize<U32>);
static_assert(!supports_forgetful_resize<ConstructedValue>);
static_assert(!supports_forgetful_resize<DestructedValue>);
static_assert(!supports_forgetful_resize<UnconstructibleValue>);

class ResizeValue {
 public:
  inline static Count instances = 0;

  ResizeValue() { ++instances; }
  ResizeValue(const ResizeValue& rhs) : value(rhs.value) { ++instances; }
  ~ResizeValue() { --instances; }

  U32 value = 42;
};

// Shrinking ends the removed lifetimes. Growing within capacity must begin
// them again just as growth into a new allocation does, preserving survivors.
PERIMORTEM_UNIT_TEST(DynamicVector, resize_lifetimes) {
  {
    Dynamic::Vector<ResizeValue> values;
    values.resize(3);
    EXPECT_EQ(ResizeValue::instances, Count(3));
    values[0].value = 7;

    values.resize(1);
    EXPECT_EQ(ResizeValue::instances, Count(1));
    values.resize(3);
    EXPECT_EQ(ResizeValue::instances, Count(3));
    EXPECT_EQ(values[1].value, U32(42));

    values.resize(100);
    EXPECT_EQ(ResizeValue::instances, Count(100));
    EXPECT_EQ(values[0].value, U32(7));
  }

  EXPECT_EQ(ResizeValue::instances, Count(0));
}

PERIMORTEM_UNIT_TEST(DynamicVector, forgetful_scalars) {
  Dynamic::Vector<U32> values;
  values.forgetful_resize(100);
  EXPECT_EQ(values.get_size(), Count(100));
  EXPECT(values.get_capacity() >= 100);

  values[99] = 42;
  EXPECT_EQ(values[99], U32(42));
}

PERIMORTEM_UNIT_TEST(DynamicVector, remove) {
  Dynamic::Vector<S32> values;
  values.insert(1);
  values.insert(2);
  values.insert(3);
  values.insert(4);

  EXPECT(values.remove(1));
  EXPECT_EQ(values.get_size(), 3);
  EXPECT_EQ(values[0], 1);
  EXPECT_EQ(values[1], 4);
  EXPECT_EQ(values[2], 3);
  EXPECT(!values.contains(2));
  EXPECT(!values.remove(3));

  EXPECT(values.remove(0));
  EXPECT_EQ(values[0], 3);
  EXPECT_EQ(values[1], 4);
  EXPECT(values.remove(1));
  EXPECT_EQ(values[0], 3);
  EXPECT(values.remove(0));
  EXPECT_EQ(values.get_size(), Count(0));
  EXPECT(!values.remove(0));
}

PERIMORTEM_UNIT_TEST(DynamicVector, remove_stable) {
  Dynamic::Vector<S32> values;
  values.insert(1);
  values.insert(2);
  values.insert(3);
  values.insert(4);

  EXPECT(values.remove_stable(1));
  EXPECT_EQ(values.get_size(), 3);
  EXPECT_EQ(values[0], 1);
  EXPECT_EQ(values[1], 3);
  EXPECT_EQ(values[2], 4);
  EXPECT(!values.contains(2));
  EXPECT(!values.remove_stable(3));

  EXPECT(values.remove_stable(0));
  EXPECT_EQ(values[0], 3);
  EXPECT_EQ(values[1], 4);
  EXPECT(values.remove_stable(1));
  EXPECT_EQ(values[0], 3);
  EXPECT(values.remove_stable(0));
  EXPECT_EQ(values.get_size(), Count(0));
  EXPECT(!values.remove_stable(0));
}

// Byte copies can preserve the resource count while leaving a pointer into the
// old object. Each constructor establishes the new address so removal has to
// preserve both the surviving values and their internal relationships.
template <bool copyable>
class RelocationValue {
 public:
  RelocationValue(U32 value, U32& owners) : value(value), owners(&owners) {
    ++owners;
  }

  RelocationValue(const RelocationValue& other)
    requires(copyable)
      : value(other.value), owners(other.owners) {
    ++*owners;
  }

  RelocationValue(RelocationValue&& other)
    requires(!copyable)
      : value(other.value) {
    Data::swap(owners, other.owners);
  }

  ~RelocationValue() {
    if (owners) {
      --*owners;
    }
  }

  const U32 value;
  const RelocationValue* const location = this;

 private:
  U32* owners = nullptr;
};

template <bool copyable>
static auto check_removal_lifetimes(Test::TestResult& result) -> void {
  for (Count stable = 0; stable < 2; ++stable) {
    Static::Vector<U32, 4> owners;
    {
      Dynamic::Vector<RelocationValue<copyable>> values;
      for (U32 index = 0; index < owners.get_size(); ++index) {
        RelocationValue<copyable> value(index, owners[index]);
        if constexpr (copyable) {
          values.insert(value);
        } else {
          values.emplace(Data::take(value));
        }
      }

      auto remove = [&](Count index) {
        return stable ? values.remove_stable(index) : values.remove(index);
      };
      EXPECT(!remove(4));
      EXPECT_EQ(values.get_size(), Count(4));
      for (Count index = 0; index < owners.get_size(); ++index) {
        EXPECT_EQ(owners[index], U32(1));
      }

      // Removing from the middle and then the front exercises both ordering
      // policies while keeping resource counts tied to the original values.
      EXPECT(remove(1));
      EXPECT_EQ(values.get_size(), Count(3));
      EXPECT_EQ(values[0].value, U32(0));
      EXPECT_EQ(values[1].value, stable ? U32(2) : U32(3));
      EXPECT_EQ(values[2].value, stable ? U32(3) : U32(2));
      EXPECT_EQ(owners[1], U32(0));
      for (Count index = 0; index < values.get_size(); ++index) {
        EXPECT(values[index].location == &values[index]);
        EXPECT_EQ(owners[values[index].value], U32(1));
      }

      EXPECT(remove(0));
      EXPECT_EQ(values.get_size(), Count(2));
      EXPECT_EQ(values[0].value, U32(2));
      EXPECT_EQ(values[1].value, U32(3));
      EXPECT_EQ(owners[0], U32(0));
      for (Count index = 0; index < values.get_size(); ++index) {
        EXPECT(values[index].location == &values[index]);
        EXPECT_EQ(owners[values[index].value], U32(1));
      }

      // The last value has no replacement. Its lifetime still ends exactly
      // once, including when removing it leaves the vector empty.
      EXPECT(remove(1));
      EXPECT_EQ(values.get_size(), Count(1));
      EXPECT_EQ(values[0].value, U32(2));
      EXPECT_EQ(owners[3], U32(0));
      EXPECT(remove(0));
      EXPECT_EQ(values.get_size(), Count(0));
      EXPECT(!remove(0));
    }

    for (Count index = 0; index < owners.get_size(); ++index) {
      EXPECT_EQ(owners[index], U32(0));
    }
  }
}

PERIMORTEM_UNIT_TEST(DynamicVector, copy_removal_lifetimes) {
  check_removal_lifetimes<true>(result);
}

PERIMORTEM_UNIT_TEST(DynamicVector, move_removal_lifetimes) {
  check_removal_lifetimes<false>(result);
}

// Exclusive owners cannot be copied during growth. Moving an element must
// transfer its obligation before the old allocation is destroyed, including
// when several capacity increases happen before the owner is finally removed.
class MoveOnly {
 public:
  explicit MoveOnly(U32& releases) : releases(&releases) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&& other) { Data::swap(releases, other.releases); }

  ~MoveOnly() {
    if (releases) {
      ++*releases;
    }
  }

 private:
  U32* releases = nullptr;
};

PERIMORTEM_UNIT_TEST(DynamicVector, move_only) {
  U32 releases[64] = {};
  {
    Dynamic::Vector<MoveOnly> values;
    for (U32 index = 0; index < 64; ++index) {
      values.emplace(MoveOnly(releases[index]));
    }

    for (U32 count : releases) {
      EXPECT_EQ(count, U32(0));
    }
  }

  for (U32 count : releases) {
    EXPECT_EQ(count, U32(1));
  }
}
