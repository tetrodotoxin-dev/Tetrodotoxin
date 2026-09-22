// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/model/image.hpp"

#include "perimortem/core/time.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/model/execution/fields/value.hpp"
#include "tetrodotoxin/model/execution/functions/function.hpp"
#include "tetrodotoxin/model/execution/layouts/sequence.hpp"
#include "tetrodotoxin/model/execution/statements/return.hpp"
#include "tetrodotoxin/model/execution/values/literal.hpp"
#include "tetrodotoxin/model/execution/values/parameter.hpp"
#include "tetrodotoxin/model/type/primitives/scalar.hpp"
#include "tetrodotoxin/model/type/primitives/structure.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Tetrodotoxin::Model;
using namespace Tetrodotoxin::Terminal;

static Validation::Harness Carriers = {
  .name = "Model::Execution::Carriers"_view};
static constexpr System::Uuid operation(0x894a52b6b85a4a82, 0xb81291620d9ca746);

// Each compile returns after all source objects have died. Both parameter
// copying and constant materialization must own their resulting code and forms.
// The terminal has no numeric family switch for any of these carriers.
template <typename Value>
static auto compile(Value value, Bool constant)
    -> Utility::Result<Llvm::Execution, Binding::Failure> {
  const Type::Primitives::Scalar<Value> type;
  const Tetrodotoxin::Model::Execution::Fields::Value field(
      Abstract::provide(type));
  const Abstract fields[] = {Abstract::provide(field)};
  const Tetrodotoxin::Model::Execution::Layouts::Sequence parameters(
      {fields, 1});
  const Tetrodotoxin::Model::Execution::Values::Parameter use(fields[0]);
  const Tetrodotoxin::Model::Execution::Values::Literal<Value> literal(
      Abstract::provide(type), value);
  const Abstract values[] = {
    constant ? Abstract::provide(literal) : Abstract::provide(use)};
  const Tetrodotoxin::Model::Execution::Layouts::Sequence returns({values, 1});
  const Tetrodotoxin::Model::Execution::Statements::Return body(
      returns.get_interface());
  const Tetrodotoxin::Model::Execution::Functions::Function function(
      operation, parameters.get_interface(), parameters.get_interface(),
      Abstract::provide(body));
  return Llvm::Execution::compile(Abstract::provide(function));
}

template <typename Value>
static auto check(Value input, Validation::Test::TestResult& result) -> void {
  for (Count mode = 0; mode < 2; ++mode) {
    compile(input, mode != 0)
        .visit(
            [&](Llvm::Execution& artifact) {
              Validation::ModelTests::Image image(artifact);
              ASSERT(image.is_set());
              Ttx::Semantic::Realization::Invocation invocation;
              ASSERT(
                  invocation.connect(
                      image.get_query(), operation, artifact.get_inputs(),
                      artifact.get_outputs()) == Binding::Status::Satisfied);
              Value output = Value(0);
              EXPECT(
                  invocation.invoke(&input, &output) ==
                  Ttx::Data::Status::Success);
              EXPECT_EQ(output, input);
            },
            [&](Binding::Failure) { EXPECT(False); });
  }
}

PERIMORTEM_UNIT_TEST(Carriers, scalar_lowering) {
  check(U8(239), result);
  check(U16(4097), result);
  check(U32(4000000001), result);
  check(U64(0xfedcba9876543210), result);
  check(S8(-101), result);
  check(S16(-24000), result);
  check(S32(-2000000001), result);
  check(S64(-9000000000000000000LL), result);
  check(R32(-2.25), result);
  check(R64(1.125), result);
  check(true, result);
}

struct Record {
  U16 key;
  R64 coordinates[3];
};

// A frame contains a nested record plus a signed scalar. Separate output fields
// reverse their order, so copying the whole input buffer would be incorrect.
// This checks both composition's padding and the terminal's occurrence map.
struct Input {
  Record record;
  S32 delta;
};
struct Output {
  S32 delta;
  Record record;
};
TTX_DATA_RECORD(
    Record,
    TTX_DATA_MEMBER(Record, key),
    TTX_DATA_MEMBER(Record, coordinates));
TTX_DATA_RECORD(
    Input,
    TTX_DATA_MEMBER(Input, record),
    TTX_DATA_MEMBER(Input, delta));
TTX_DATA_RECORD(
    Output,
    TTX_DATA_MEMBER(Output, delta),
    TTX_DATA_MEMBER(Output, record));

PERIMORTEM_UNIT_TEST(Carriers, heterogeneous_frame) {
  using namespace Tetrodotoxin::Model::Execution;
  const auto& form = Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<Record>::reference>::get_representation();
  const Type::Primitives::Structure record(form);
  const Type::Primitives::Scalar<S32> signed_type;
  const Fields::Value a(Abstract::provide(record));
  const Fields::Value b(Abstract::provide(signed_type));
  const Abstract inputs[] = {Abstract::provide(a), Abstract::provide(b)};
  const Abstract outputs[] = {inputs[1], inputs[0]};
  const Layouts::Sequence parameters({inputs, 2});
  const Layouts::Sequence results({outputs, 2});
  const Values::Parameter first(inputs[0]);
  const Values::Parameter second(inputs[1]);
  const Abstract returned[] = {
    Abstract::provide(second), Abstract::provide(first)};
  const Layouts::Sequence values({returned, 2});
  const Statements::Return body(values.get_interface());
  const Functions::Function function(
      operation, parameters.get_interface(), results.get_interface(),
      Abstract::provide(body));

  Llvm::Execution::compile(Abstract::provide(function))
      .visit(
          [&](Llvm::Execution& artifact) {
            EXPECT_EQ(artifact.get_inputs().get_extent(), Count(sizeof(Input)));
            EXPECT_EQ(
                artifact.get_outputs().get_extent(), Count(sizeof(Output)));
            Validation::ModelTests::Image image(artifact);
            ASSERT(image.is_set());
            Ttx::Semantic::Realization::Invocation invocation;
            ASSERT(
                invocation.connect(
                    image.get_query(), operation, artifact.get_inputs(),
                    artifact.get_outputs()) == Binding::Status::Satisfied);
            const Input input(Record(17, {1.25, -3.5, 8.75}), -42);
            Output output;
            EXPECT(
                invocation.invoke(&input, &output) ==
                Ttx::Data::Status::Success);
            EXPECT_EQ(output.delta, input.delta);
            EXPECT_EQ(output.record.key, input.record.key);
            for (Count i = 0; i < 3; ++i) {
              EXPECT_EQ(
                  output.record.coordinates[i], input.record.coordinates[i]);
            }
          },
          [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(Carriers, wide_frame) {
  using namespace Tetrodotoxin::Model::Execution;
  struct ObservedType {
    Type::Primitives::Scalar<U32> implementation;
    mutable Count queries = 0;
    auto get_data() const -> Core::View::Bytes { return {}; }
    auto supports(System::Uuid id) const -> Binding::Status {
      ++queries;
      return implementation.supports(id);
    }
    auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage output) const
        -> Binding::Status {
      ++queries;
      return implementation.bind_interface(id, output);
    }
  } type;
  Memory::Allocator::Arena arena;
  Memory::Dynamic::Vector<Abstract> fields;
  Memory::Dynamic::Vector<Abstract> returned;
  constexpr Count width = 512;
  for (Count i = 0; i < width; ++i) {
    const auto field = Abstract::provide(
        arena.construct<Fields::Value>(Abstract::provide(type)));
    fields.insert(field);
    returned.insert(
        Abstract::provide(arena.construct<Values::Parameter>(field)));
  }

  const Layouts::Sequence parameters(fields.get_view());
  const Layouts::Sequence values(returned.get_view());
  const Statements::Return body(values.get_interface());
  const Functions::Function function(
      operation, parameters.get_interface(), parameters.get_interface(),
      Abstract::provide(body));
  const auto start = Core::Time::clock();
  Llvm::Execution::compile(Abstract::provide(function))
      .visit(
          [&](Llvm::Execution& artifact) {
            const auto duration =
                start.measure(Core::Time::clock()).convert_to_nanoseconds();
            // Each slot asks Storage twice for the two frames, Conversion once,
            // then one property and one Storage question for the actual value.
            // The 512 field observation never recursively rediscovers
            // peers.
            EXPECT_EQ(type.queries, width * 5);
            const auto queries = type.queries;
            Validation::ModelTests::Image image(artifact);
            ASSERT(image.is_set());
            Ttx::Semantic::Realization::Invocation invocation;
            ASSERT(
                invocation.connect(
                    image.get_query(), operation, artifact.get_inputs(),
                    artifact.get_outputs()) == Binding::Status::Satisfied);
            U32 input[width];
            U32 output[width];
            for (Count i = 0; i < width; ++i) {
              input[i] = U32(i * 7);
            }
            const auto warm_start = Core::Time::clock();
            for (Count i = 0; i < 10000; ++i) {
              ASSERT(
                  invocation.invoke(input, output) ==
                  Ttx::Data::Status::Success);
            }
            const auto warm = warm_start.measure(Core::Time::clock())
                                  .convert_to_nanoseconds();
            EXPECT_EQ(type.queries, queries);
            for (Count i = 0; i < width; ++i) {
              EXPECT_EQ(output[i], input[i]);
            }
            Core::Static::Bytes<192> buffer;
            Core::Writer::Textual text(buffer);
            text << "512-field compile: "_view << duration
                 << " ns, 10000 retained calls: "_view << warm << " ns"_view;
            Validation::Test::log_message("Model::Execution"_view, 0, text);
          },
          [&](Binding::Failure) { EXPECT(False); });
}
