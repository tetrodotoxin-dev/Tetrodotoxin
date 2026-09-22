// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/execution.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/model/image.hpp"

#include "tetrodotoxin/model/execution/fields/value.hpp"
#include "tetrodotoxin/model/execution/functions/function.hpp"
#include "tetrodotoxin/model/execution/layouts/sequence.hpp"
#include "tetrodotoxin/model/execution/statements/return.hpp"
#include "tetrodotoxin/model/execution/values/parameter.hpp"
#include "tetrodotoxin/model/type/primitives/u32.hpp"
#include "ttx/semantic/realization/invocation.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Tetrodotoxin;

static Validation::Harness Models = {.name = "Model::Execution"_view};
static constexpr System::Uuid operation(0x894a52b6b85a4a82, 0xb81291620d9ca746);

static auto identity()
    -> Utility::Result<Terminal::Llvm::Execution, Binding::Failure> {
  const Model::Type::Primitives::U32 type;
  const Tetrodotoxin::Model::Execution::Fields::Value field(
      Abstract::provide(type));
  const Abstract fields[] = {Abstract::provide(field)};
  const Tetrodotoxin::Model::Execution::Layouts::Sequence parameters(
      {fields, 1});
  const Tetrodotoxin::Model::Execution::Values::Parameter value(fields[0]);
  const Abstract values[] = {Abstract::provide(value)};
  const Tetrodotoxin::Model::Execution::Layouts::Sequence returned({values, 1});
  const Tetrodotoxin::Model::Execution::Statements::Return body(
      returned.get_interface());
  const Tetrodotoxin::Model::Execution::Functions::Function function(
      operation, parameters.get_interface(), parameters.get_interface(),
      Abstract::provide(body));
  return Terminal::Llvm::Execution::compile(Abstract::provide(function));
}

PERIMORTEM_UNIT_TEST(Models, detached_execution) {
  // Every model object above has died before this invocation. Compilation may
  // retain only its generated code and copied frame descriptions.
  identity().visit(
      [&](Terminal::Llvm::Execution& compiled) {
        Validation::ModelTests::Image image(compiled);
        ASSERT(image.is_set());
        Ttx::Semantic::Realization::Invocation invoke;
        ASSERT(
            invoke.connect(
                image.get_query(), operation, compiled.get_inputs(),
                compiled.get_outputs()) == Binding::Status::Satisfied);
        U32 input = 1947;
        U32 output = 0;
        EXPECT(invoke.invoke(&input, &output) == Ttx::Data::Status::Success);
        EXPECT_EQ(output, input);
      },
      [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(Models, rejected_invocation) {
  identity().visit(
      [&](Terminal::Llvm::Execution& artifact) {
        Validation::ModelTests::Image image(artifact);
        ASSERT(image.is_set());
        Ttx::Semantic::Realization::Invocation invoke;
        const auto& wrong = Ttx::Data::Form::Compiled<
            Ttx::Data::Form::Native<U16>::reference>::get_representation();
        EXPECT(
            invoke.connect(
                image.get_query(), operation, wrong, artifact.get_outputs()) ==
            Binding::Status::Rejected);
        EXPECT(
            invoke.connect(
                image.get_query(), System::Uuid(0, 0), artifact.get_inputs(),
                artifact.get_outputs()) == Binding::Status::Unsupported);
      },
      [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(Models, ordered_returns) {
  const Model::Type::Primitives::U32 type;
  const Model::Execution::Fields::Value first(Abstract::provide(type));
  const Model::Execution::Fields::Value second(Abstract::provide(type));
  const Abstract fields[] = {
    Abstract::provide(first), Abstract::provide(second)};
  const Model::Execution::Layouts::Sequence parameters({fields, 2});
  const Model::Execution::Values::Parameter left(fields[0]);
  const Model::Execution::Values::Parameter right(fields[1]);
  const Abstract reversed[] = {
    Abstract::provide(right), Abstract::provide(left)};
  const Model::Execution::Layouts::Sequence values({reversed, 2});
  const Model::Execution::Statements::Return body(values.get_interface());
  const Model::Execution::Functions::Function function(
      operation, parameters.get_interface(), parameters.get_interface(),
      Abstract::provide(body));

  // Field identity establishes each parameter position. Return order is a
  // separate execution fact, and the terminal realizes both into byte offsets.
  Terminal::Llvm::Execution::compile(Abstract::provide(function))
      .visit(
          [&](auto& artifact) {
            Validation::ModelTests::Image image(artifact);
            ASSERT(image.is_set());
            Ttx::Semantic::Realization::Invocation invocation;
            ASSERT(
                invocation.connect(
                    image.get_query(), operation, artifact.get_inputs(),
                    artifact.get_outputs()) == Binding::Status::Satisfied);
            const U32 input[] = {7, 31};
            U32 output[] = {0, 0};
            EXPECT(
                invocation.invoke(input, output) == Ttx::Data::Status::Success);
            EXPECT_EQ(output[0], U32(31));
            EXPECT_EQ(output[1], U32(7));
          },
          [&](Binding::Failure) { EXPECT(False); });
}
