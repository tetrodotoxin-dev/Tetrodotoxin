// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/model/execution/statements/block.hpp"

#include "tests/model/fixtures/ordered.h"
#include "tests/model/image.hpp"
#include "tetrodotoxin/model/execution/fields/value.hpp"
#include "tetrodotoxin/model/execution/functions/function.hpp"
#include "tetrodotoxin/model/execution/layouts/sequence.hpp"
#include "tetrodotoxin/model/execution/statements/return.hpp"
#include "tetrodotoxin/model/execution/values/literal.hpp"
#include "tetrodotoxin/model/type/primitives/u32.hpp"
#include "toolchain/validation/unit_test.hpp"
#include "ttx/concept/answers/unknown.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Tetrodotoxin::Model;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Model::Execution;

static Toolchain::Validation::Harness Blocks = {
  .name = "Model::Execution::Block"};
static constexpr System::Uuid operation(0xd0b10cadb70a4d7a, 0x9b785e423d73a051);

static auto compile(Abstract body, Bool returns_value)
    -> Utility::Result<Llvm::Execution, Binding::Failure> {
  const Type::Primitives::U32 type;
  const Fields::Value field(Abstract::provide(type));
  const Abstract fields[] = {Abstract::provide(field)};
  const Layouts::Sequence inputs;
  const Layouts::Sequence outputs(
      Core::View::Vector<Abstract>(fields, returns_value ? 1 : 0));
  const Functions::Function function(
      operation, inputs.get_interface(), outputs.get_interface(), body);
  return Llvm::Execution::compile(Abstract::provide(function));
}

static auto fixture() -> ordered_fixture {
  return ordered_fixture(
      &Binding::representation<Abstract>(), nullptr, 0, TTX_BINDING_SATISFIED,
      TTX_BINDING_UNKNOWN, 0, 0);
}

VALIDATION_TEST(Blocks, publication_contracts) {
  const Statements::Block block;
  const auto subject = Abstract::provide(block);
  const auto query = subject.get_query();
  EXPECT(query.supports<Abstract>() == Binding::Status::Satisfied);
  // Use Query rather than Abstract's local bind shortcut to exercise the
  // published base contract and its actual representation negotiation.
  query.bind<Abstract>().visit(
      [&](Abstract bound) { EXPECT(bound == subject); },
      [&](Binding::Failure) { EXPECT(False); });
  query.bind<Policies::Ordered>().visit(
      [&](Policies::Ordered) {}, [&](Binding::Failure) { EXPECT(False); });

  const System::Uuid unrelated(0x564a3d18ad194a03, 0xaceb616c9ac270a9);
  EXPECT(query.supports(unrelated) == Binding::Status::Unknown);
  const auto& form = Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<U32>::reference>::get_representation();
  U32 sentinel = 0x53ad127e;
  const Ttx::Data::Form::Storage target(
      ttx_storage(&form, reinterpret_cast<U8*>(&sentinel), sizeof(sentinel)));
  EXPECT(query.bind(unrelated, target) == Binding::Status::Unknown);
  EXPECT_EQ(sentinel, U32(0x53ad127e));

  // An invalid marker destination rejects acquisition, not the subject's
  // ordering promise. Only the separate supports answer describes that policy.
  EXPECT(
      query.bind(Policies::Ordered::contract_id, target) ==
      Binding::Status::Rejected);
  EXPECT(query.supports<Policies::Ordered>() == Binding::Status::Satisfied);
  EXPECT_EQ(sentinel, U32(0x53ad127e));
}

VALIDATION_TEST(Blocks, nested_foreign_return) {
  const auto compiled =
      [&]() -> Utility::Result<Llvm::Execution, Binding::Failure> {
    const Type::Primitives::U32 type;
    const Values::Literal<U32> first(Abstract::provide(type), 7);
    const Values::Literal<U32> second(Abstract::provide(type), 31);
    const Abstract values[] = {
      Abstract::provide(first), Abstract::provide(second)};
    const Layouts::Sequence first_values({values, 1});
    const Layouts::Sequence second_values({values + 1, 1});
    const Statements::Return returned(first_values.get_interface());
    const Statements::Return unreachable(second_values.get_interface());
    const Statements::Block empty;
    const ttx_abstract children[] = {
      Abstract::provide(empty).get_abi(), Abstract::provide(returned).get_abi(),
      Answers::Unknown::get_unknown().get_abi()};
    auto foreign = fixture();
    foreign.children = children;
    foreign.count = 3;
    const Abstract statements[] = {
      Abstract(ordered_fixture_view(&foreign)), Abstract::provide(unreachable)};
    const Statements::Block outer({statements, 2});
    auto result = compile(Abstract::provide(outer), True);
    // Inspected below through successful execution. The fixture cannot supply
    // a bound Ordered API, so a successful compile used supports and
    // visitation.
    if (foreign.visits != 1 || foreign.marker_bindings != 0) {
      return Binding::Failure::Rejected;
    }
    return result;
  }();
  compiled.visit(
      [&](const Llvm::Execution& artifact) {
        // Every source subject and both native and C sequence owners are gone.
        Validation::ModelTests::Image image(artifact);
        ASSERT(image.is_set());
        Ttx::Semantic::Realization::Invocation invoke;
        ASSERT(
            invoke.connect(
                image.get_query(), operation, artifact.get_inputs(),
                artifact.get_outputs()) == Binding::Status::Satisfied);
        U32 output = 0;
        EXPECT(invoke.invoke(nullptr, &output) == Ttx::Data::Status::Success);
        EXPECT_EQ(output, U32(7));
      },
      [&](Binding::Failure) { EXPECT(False); });
}

VALIDATION_TEST(Blocks, admission_is_not_binding) {
  auto foreign = fixture();
  const Abstract subject(ordered_fixture_view(&foreign));
  const ttx_binding_status outcomes[] = {
    TTX_BINDING_UNKNOWN, TTX_BINDING_REJECTED};
  for (const auto status : outcomes) {
    foreign.ordered_status = status;
    compile(subject, False)
        .visit(
            [&](Llvm::Execution&) { EXPECT(False); },
            [&](Binding::Failure failure) {
              EXPECT_EQ(static_cast<ttx_binding_status>(failure), status);
            });
    EXPECT_EQ(foreign.visits, Count(0));
  }
  foreign.ordered_status = TTX_BINDING_SATISFIED;
  foreign.return_status = TTX_BINDING_REJECTED;
  compile(subject, False)
      .visit(
          [&](Llvm::Execution&) { EXPECT(False); },
          [&](Binding::Failure failure) {
            EXPECT(failure == Binding::Failure::Rejected);
          });
  EXPECT_EQ(foreign.visits, Count(0));
  foreign.return_status = TTX_BINDING_UNKNOWN;
  compile(subject, False)
      .visit(
          [&](Llvm::Execution&) {}, [&](Binding::Failure) { EXPECT(False); });
  EXPECT_EQ(foreign.visits, Count(1));
  EXPECT_EQ(foreign.marker_bindings, Count(0));
}

VALIDATION_TEST(Blocks, empty_and_unknown) {
  const Statements::Block empty;
  compile(Abstract::provide(empty), True)
      .visit(
          [&](Llvm::Execution&) { EXPECT(False); },
          [&](Binding::Failure failure) {
            EXPECT(failure == Binding::Failure::Rejected);
          });
  compile(Answers::Unknown::get_unknown(), False)
      .visit(
          [&](Llvm::Execution&) { EXPECT(False); },
          [&](Binding::Failure failure) {
            EXPECT(failure == Binding::Failure::Unknown);
          });
}

VALIDATION_TEST(Blocks, occurrence_and_recursion) {
  auto foreign = fixture();
  const Abstract child(ordered_fixture_view(&foreign));
  const Abstract children[] = {child, child};
  const Statements::Block outer({children, 2});
  compile(Abstract::provide(outer), False)
      .visit(
          [&](Llvm::Execution&) {}, [&](Binding::Failure) { EXPECT(False); });
  EXPECT_EQ(foreign.visits, Count(2));
  const auto self = child.get_abi();
  foreign.children = &self;
  foreign.count = 1;
  compile(child, False)
      .visit(
          [&](Llvm::Execution&) { EXPECT(False); },
          [&](Binding::Failure failure) {
            EXPECT(failure == Binding::Failure::Rejected);
          });
}
