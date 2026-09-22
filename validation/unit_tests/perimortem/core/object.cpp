// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/object.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/record.hpp"

#include "perimortem/abi/core/object.hpp"

using namespace Perimortem::Memory;
using namespace Validation;

static Harness CoreObject = {
  .name = "Core::Object and Memory::Dynamic::Record"_view,
};

class RaiiProbe {
 public:
  RaiiProbe(Count& destructor_count, Count value = 0)
      : destructor_count(destructor_count), value(value) {}

  ~RaiiProbe() { destructor_count++; }

  constexpr auto get_value() const -> Count { return value; }

 private:
  Count& destructor_count;
  Count value = 0;
};

static_assert(
    sizeof(Dynamic::Record<RaiiProbe>) == sizeof(Perimortem::Core::Object<>));

static Count native_finalizations = 0;

static auto finalize_native_object(U8*) -> void {
  native_finalizations++;
}

static constexpr Perimortem::Core::Object<>::Descriptor
    native_descriptor(sizeof(U64), alignof(U64), finalize_native_object);

PERIMORTEM_UNIT_TEST(CoreObject, native_surface) {
  native_finalizations = 0;
  U8* object = perimortem_core_object_allocate(&native_descriptor);
  const Perimortem::Core::Object<>::Descriptor& descriptor =
      Perimortem::Core::Object<>(object).get_descriptor();
  EXPECT_EQ(descriptor.get_size(), Count(sizeof(U64)));
  EXPECT_EQ(descriptor.get_alignment(), Count(alignof(U64)));
  EXPECT(descriptor.get_finalizer() == finalize_native_object);
  perimortem_core_object_retain(object);
  perimortem_core_object_release(object);
  EXPECT_EQ(native_finalizations, Count(0));
  perimortem_core_object_release(object);
  EXPECT_EQ(native_finalizations, Count(1));
}

PERIMORTEM_UNIT_TEST(CoreObject, empty_surface) {
  perimortem_core_object_retain({});
  perimortem_core_object_release({});
  EXPECT_EQ(perimortem_core_object_capacity({}), Count(0));
}

PERIMORTEM_UNIT_TEST(CoreObject, shared_buffer_access) {
  Perimortem::Core::Object<U64> first(3);
  auto initialized = first.get_access();
  ASSERT(initialized.get_size() >= 3);
  auto first_value = initialized[0];
  ASSERT(first_value);
  *first_value = 42;

  Perimortem::Core::Object<U64> second = first;
  EXPECT(first.is_shared());
  EXPECT(second.is_shared());
  auto shared = second.get_access();
  auto second_value = shared[0];
  ASSERT(second_value);
  *second_value = 7;

  EXPECT_EQ(first.get_view()[0], U64(7));
  EXPECT_EQ(second.get_view()[0], U64(7));
  EXPECT(first.get_view().get_data() == second.get_view().get_data());

  second.clone();
  EXPECT_NOT(first.is_shared());
  EXPECT_NOT(second.is_shared());
  EXPECT(first.get_view().get_data() != second.get_view().get_data());
  auto cloned = second.get_access()[0];
  ASSERT(cloned);
  *cloned = 9;
  EXPECT_EQ(first.get_view()[0], U64(7));
  EXPECT_EQ(second.get_view()[0], U64(9));
}

PERIMORTEM_UNIT_TEST(CoreObject, empty_option_payload) {
  Perimortem::Core::Object<U8> empty;
  Perimortem::Core::Option<Perimortem::Core::Object<U8>> selected(
      static_cast<Perimortem::Core::Object<U8>&&>(empty));

  EXPECT(selected);
  EXPECT((*selected).is_empty());
  EXPECT(sizeof(selected) > sizeof(Perimortem::Core::Object<U8>));
}

PERIMORTEM_UNIT_TEST(CoreObject, shared_lifetime) {
  Count destructor_count = 0;

  {
    Dynamic::Record<RaiiProbe> probe(destructor_count, 42);
    EXPECT_EQ(probe->get_value(), Count(42));

    {
      Dynamic::Record<RaiiProbe> second = probe;
      EXPECT_EQ(second->get_value(), Count(42));
      EXPECT_EQ(destructor_count, Count(0));
    }

    EXPECT_EQ(destructor_count, Count(0));
  }

  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(CoreObject, assignment) {
  Count destructor_count = 0;

  {
    Dynamic::Record<RaiiProbe> first(destructor_count, 1);
    Dynamic::Record<RaiiProbe> second(destructor_count, 2);

    second = first;
    EXPECT_EQ(destructor_count, Count(1));
    EXPECT_EQ(second->get_value(), Count(1));
  }

  EXPECT_EQ(destructor_count, Count(2));
}

PERIMORTEM_UNIT_TEST(CoreObject, assignment_reserves) {
  Count destructor_count = 0;

  {
    Dynamic::Record<RaiiProbe> first(destructor_count, 3);
    Dynamic::Record<RaiiProbe> second = first;
    const Dynamic::Record<RaiiProbe>& alias = first;

    first = alias;
    second = first;
    EXPECT_EQ(destructor_count, Count(0));
  }

  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(CoreObject, move_assignment) {
  Count destructor_count = 0;

  {
    Dynamic::Record<RaiiProbe> first(destructor_count, 1);
    {
      Dynamic::Record<RaiiProbe> second(destructor_count, 2);

      first = static_cast<Dynamic::Record<RaiiProbe>&&>(second);
      EXPECT_EQ(first->get_value(), Count(2));
      EXPECT_EQ(destructor_count, Count(0));
    }

    EXPECT_EQ(first->get_value(), Count(2));
    EXPECT_EQ(destructor_count, Count(1));
  }

  EXPECT_EQ(destructor_count, Count(2));
}

PERIMORTEM_UNIT_TEST(CoreObject, move_construction) {
  Count destructor_count = 0;

  {
    Dynamic::Record<RaiiProbe> first(destructor_count, 3);
    {
      Dynamic::Record<RaiiProbe> second(
          static_cast<Dynamic::Record<RaiiProbe>&&>(first));
      EXPECT_EQ(second->get_value(), Count(3));
    }

    EXPECT_EQ(destructor_count, Count(0));
  }

  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(CoreObject, map_owner) {
  Count destructor_count = 0;
  Dynamic::Map<Count, Dynamic::Record<RaiiProbe>> values;

  {
    Dynamic::Record<RaiiProbe> probe(destructor_count, 7);
    values.insert(0, probe);
  }

  EXPECT_EQ(destructor_count, Count(0));
  auto found = values.find(0);
  ASSERT(found);
  EXPECT_EQ((*found).value->get_value(), Count(7));

  values.remove(0);
  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(CoreObject, map_rehash) {
  Count destructor_count = 0;
  Dynamic::Map<Count, Dynamic::Record<RaiiProbe>> values;
  for (Count i = 0; i < 16; i++) {
    Dynamic::Record<RaiiProbe> probe(destructor_count, i);
    values.insert(i, probe);
  }

  EXPECT_EQ(destructor_count, Count(0));
  for (Count i = 0; i < 16; i++) {
    auto found = values.find(i);
    ASSERT(found);
    EXPECT_EQ((*found).value->get_value(), i);
  }

  values.clear();
  EXPECT_EQ(destructor_count, Count(16));
}
