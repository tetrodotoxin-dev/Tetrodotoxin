// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/model/execution/assignments/memory.hpp"
#include "tetrodotoxin/model/execution/values/literal.hpp"
#include "tetrodotoxin/model/type/policies/conversion.hpp"
#include "tetrodotoxin/model/type/primitives/boolean.hpp"
#include "tetrodotoxin/model/type/primitives/r32.hpp"
#include "tetrodotoxin/model/type/primitives/r64.hpp"
#include "tetrodotoxin/model/type/primitives/s16.hpp"
#include "tetrodotoxin/model/type/primitives/s32.hpp"
#include "tetrodotoxin/model/type/primitives/s64.hpp"
#include "tetrodotoxin/model/type/primitives/s8.hpp"
#include "tetrodotoxin/model/type/primitives/structure.hpp"
#include "tetrodotoxin/model/type/primitives/u16.hpp"
#include "tetrodotoxin/model/type/primitives/u32.hpp"
#include "tetrodotoxin/model/type/primitives/u64.hpp"
#include "tetrodotoxin/model/type/primitives/u8.hpp"
#include "ttx/semantic/flows/copy.hpp"
#include "ttx/semantic/transport/block.hpp"
#include "ttx/semantic/transport/flow.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Model;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;

static Validation::Harness Types = {.name = "Model::Type::Scalar"_view};

// Every primitive answers the same storage question. The caller asks its
// numeric property independently, then uses an ordinary negotiated Flow to
// observe a literal. No category switch discovers where to find a width.
template <typename Provider, typename Value>
static auto primitive(
    Validation::Test::TestResult& result,
    System::Uuid policy,
    Value value) -> void {
  const Provider provider;
  const Abstract type = Abstract::provide(provider);
  EXPECT(type.supports(policy) == Binding::Status::Satisfied);

  type.bind<Type::Storage>().visit(
      [&](Type::Storage storage) {
        storage.get_representation().visit(
            [&](const auto& representation) {
              EXPECT_EQ(representation.get_extent(), Count(sizeof(Value)));
              EXPECT_EQ(representation.get_alignment(), Count(alignof(Value)));

              const Execution::Values::Literal<Value> literal(type, value);
              Ttx::Semantic::Transport::Flow flow;
              ASSERT(
                  flow.connect(
                      decltype(flow)::reader(representation),
                      Abstract::provide(literal).get_query()) ==
                  decltype(flow)::Status::Success);

              Value output = Value();
              const Ttx::Data::Form::Storage destination(
                  ttx_storage{
                    &representation, reinterpret_cast<U8*>(&output),
                    sizeof(output)});
              EXPECT(
                  Ttx::Semantic::Flows::Copy::flow(flow, destination) ==
                  Ttx::Data::Status::Success);
              EXPECT_EQ(output, value);

              // Conversion is a current semantic answer. The returned source
              // is already acceptable, but that does not make the primitive
              // Type an assignable destination.
              type.bind<Type::Policies::Conversion>().visit(
                  [&](Type::Policies::Conversion conversion) {
                    conversion.convert(Abstract::provide(literal))
                        .visit(
                            [&](Abstract selected) {
                              EXPECT(selected == Abstract::provide(literal));
                            },
                            [&](Binding::Failure) { EXPECT(False); });
                  },
                  [&](Binding::Failure) { EXPECT(False); });
              EXPECT(
                  type.supports<Execution::Assignment>() ==
                  Binding::Status::Unsupported);

              output = Value();
              const Execution::Assignments::Memory location(type, destination);
              Abstract::provide(location).bind<Execution::Assignment>().visit(
                  [&](Execution::Assignment assignment) {
                    assignment.assign(Abstract::provide(literal))
                        .visit(
                            [&](Ttx::Data::Status status) {
                              EXPECT(status == Ttx::Data::Status::Success);
                            },
                            [&](Binding::Failure) { EXPECT(False); });
                  },
                  [&](Binding::Failure) { EXPECT(False); });
              EXPECT_EQ(output, value);
            },
            [&](Binding::Failure) { EXPECT(False); });
      },
      [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(Types, primitive_composition) {
  primitive<Type::Primitives::U8>(
      result, Type::Policies::Unsigned::contract_id, U8(17));
  primitive<Type::Primitives::U16>(
      result, Type::Policies::Unsigned::contract_id, U16(900));
  primitive<Type::Primitives::U32>(
      result, Type::Policies::Unsigned::contract_id, U32(12345));
  primitive<Type::Primitives::U64>(
      result, Type::Policies::Unsigned::contract_id, U64(1) << 40);
  primitive<Type::Primitives::S8>(
      result, Type::Policies::Signed::contract_id, S8(-17));
  primitive<Type::Primitives::S16>(
      result, Type::Policies::Signed::contract_id, S16(-900));
  primitive<Type::Primitives::S32>(
      result, Type::Policies::Signed::contract_id, S32(-12345));
  primitive<Type::Primitives::S64>(
      result, Type::Policies::Signed::contract_id, -(S64(1) << 40));
  primitive<Type::Primitives::R32>(
      result, Type::Policies::Real::contract_id, R32(1.25));
  primitive<Type::Primitives::R64>(
      result, Type::Policies::Real::contract_id, R64(-42.5));
  primitive<Type::Primitives::Boolean>(
      result, Type::Policies::Flag::contract_id, true);
}

// U8 and Boolean share a byte carrier. Only Boolean offers the truth operation
// here, so matching storage does not manufacture a missing semantic policy.
PERIMORTEM_UNIT_TEST(Types, truth_is_a_policy) {
  const Type::Primitives::Boolean boolean;
  const Type::Primitives::U8 integer;
  EXPECT(
      Abstract::provide(integer).supports<Type::Policies::Flag>() ==
      Binding::Status::Unsupported);
  EXPECT(
      Abstract::provide(boolean).supports<Type::Policies::Unsigned>() ==
      Binding::Status::Unsupported);

  Abstract::provide(boolean).bind<Type::Policies::Flag>().visit(
      [&](Type::Policies::Flag flag) {
        const auto& form = Type::Primitives::Boolean::get_representation();
        for (U8 value = 0; value < 2; ++value) {
          const Ttx::Data::Form::Storage observation(
              ttx_storage{&form, &value, sizeof(value)});
          flag.get_truth(observation)
              .visit(
                  [&](Bool answer) { EXPECT(answer == Bool(value)); },
                  [&](Ttx::Data::Status) { EXPECT(False); });
        }

        U32 value = 1;
        const auto& wrong = Type::Primitives::U32::get_representation();
        flag.get_truth(
                Ttx::Data::Form::Storage(
                    ttx_storage{
                      &wrong, reinterpret_cast<U8*>(&value), sizeof(value)}))
            .visit(
                [&](Bool) { EXPECT(False); },
                [&](Ttx::Data::Status status) {
                  EXPECT(status == Ttx::Data::Status::Incompatible);
                });
      },
      [&](Binding::Failure) { EXPECT(False); });
}

// Equal storage capacity alone is not permission to reinterpret a value. The
// receiving scalar requires both its numeric policy and its complete Data form.
PERIMORTEM_UNIT_TEST(Types, unrelated_domains) {
  const Type::Primitives::U32 target;
  const Type::Primitives::S32 signed_type;
  const Type::Primitives::R32 real_type;
  const Execution::Values::Literal<S32> integer(
      Abstract::provide(signed_type), -1);
  const Execution::Values::Literal<R32> real(Abstract::provide(real_type), 1.0);
  const Abstract sources[] = {
    Abstract::provide(integer), Abstract::provide(real)};
  U32 output = 123;
  const auto& form = target.get_representation();
  const Execution::Assignments::Memory memory(
      Abstract::provide(target),
      Ttx::Data::Form::Storage(
          ttx_storage(&form, reinterpret_cast<U8*>(&output), sizeof(output))));
  for (const auto source : sources) {
    memory.assign(source).visit(
        [&](Ttx::Data::Status) { EXPECT(False); },
        [&](Binding::Failure status) {
          EXPECT(status == Binding::Failure::Unsupported);
        });
    EXPECT_EQ(output, U32(123));
  }
}
