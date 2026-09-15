// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/data/protocol/fragment.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/ttx/data/form/preparation.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Data;
using namespace Ttx::Data::Form;

static Validation::Harness TtxFragment = {
  .name = "TTX::Data::Protocol::Fragment"_view};
static constexpr auto u32 = Schema::primitive(Schema::Value::U32);

// The C getter fills its output during the call. Native users receive a typed
// Result and need no reply receiver, pending state or provider buffer lifetime.
PERIMORTEM_UNIT_TEST(TtxFragment, typed_fragment_read) {
  Validation::DataTests::Preparation prepare;
  const auto& representation = prepare(u32);
  const Protocol::Fragment::Access::Operations operations = {
    .representation = [](const void* state) -> const Representation* {
      return static_cast<const Representation*>(state);
    },
    .get_u32 = [](const void*, Count coordinate,
                  U32* result) -> ttx_data_status {
      if (coordinate) {
        return TTX_DATA_BOUNDS;
      }

      *result = 42;
      return TTX_DATA_SUCCESS;
    },
  };

  Protocol::Fragment::Access access(&representation, operations);
  access.get_u32(0).visit(
      [&](U32 value) { EXPECT_EQ(value, U32(42)); },
      [&](Status) { EXPECT(false); });

  access.get_u32(4).visit(
      [&](U32) { EXPECT(false); },
      [&](Status status) { EXPECT(status == Status::Bounds); });
}

// Vector getters expose whole bit carriers through output pointers. No SIMD
// instruction set is required to exercise this protocol, and a failed read
// produces no C++ value. The declared representation fixes the carrier size.
PERIMORTEM_UNIT_TEST(TtxFragment, vector_observation) {
  Validation::DataTests::Preparation prepare;
  const auto& representation = prepare(Schema::primitive(Schema::Value::V512));
  EXPECT_EQ(representation.get_extent(), Count(64));
  EXPECT_EQ(representation.get_alignment(), Count(64));
  EXPECT_EQ(representation.get_depth(), U8(2));
  const Protocol::Fragment::Access::Operations operations = {
    .representation = [](const void* state) -> const Representation* {
      return static_cast<const Representation*>(state);
    },
    .get_v512 = [](const void*, Count offset,
                   ttx_vector512* value) -> ttx_data_status {
      if (offset) {
        return TTX_DATA_BOUNDS;
      }

      for (Count i = 0; i < 64; ++i) {
        value->bytes[i] = U8(i);
      }

      return TTX_DATA_SUCCESS;
    },
  };
  Protocol::Fragment::Access access(&representation, operations);
  access.get_v512(0).visit(
      [&](Schema::V512 value) {
        for (Count i = 0; i < 64; ++i) {
          EXPECT_EQ(value.bytes[i], U8(i));
        }
      },
      [&](Status) { EXPECT(false); });
  access.get_v512(1).visit(
      [&](Schema::V512) { EXPECT(false); },
      [&](Status status) { EXPECT(status == Status::Bounds); });
}

// Each SIMD width has its own typed C getter. The shared fixture fills a
// complete carrier without using SIMD instructions. The caller checks each
// byte and the representation's width instead of inspecting provider state.
PERIMORTEM_UNIT_TEST(TtxFragment, vector_widths) {
  Validation::DataTests::Preparation prepare;
  const auto read = [](const void*, Count, auto* result) -> ttx_data_status {
    for (Count i = 0; i < sizeof(*result); ++i) {
      result->bytes[i] = U8(i + 1);
    }
    return TTX_DATA_SUCCESS;
  };
  const Protocol::Fragment::Access::Operations operations = {
    .representation = [](const void* state) -> const Representation* {
      return static_cast<const Representation*>(state);
    },
    .get_v64 = read,
    .get_v128 = read,
    .get_v256 = read,
    .get_v512 = read,
  };
  const auto check = [&](Schema::Value type, auto observe) {
    const auto& representation = prepare(Schema::primitive(type));
    Protocol::Fragment::Access access(&representation, operations);
    observe(access).visit(
        [&](auto value) {
          EXPECT_EQ(sizeof(value), representation.get_extent());
          EXPECT_EQ(alignof(decltype(value)), representation.get_alignment());
          for (Count i = 0; i < sizeof(value); ++i) {
            EXPECT_EQ(value.bytes[i], U8(i + 1));
          }
        },
        [&](Status) { EXPECT(false); });
  };
  check(Schema::Value::V64, [](auto access) { return access.get_v64(0); });
  check(Schema::Value::V128, [](auto access) { return access.get_v128(0); });
  check(Schema::Value::V256, [](auto access) { return access.get_v256(0); });
  check(Schema::Value::V512, [](auto access) { return access.get_v512(0); });
}
