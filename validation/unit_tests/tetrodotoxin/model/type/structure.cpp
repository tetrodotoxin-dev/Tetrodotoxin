// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/model/type/primitives/structure.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/model/execution/assignments/memory.hpp"
#include "tetrodotoxin/model/execution/values/literal.hpp"
#include "tetrodotoxin/model/type/policies/unsigned.hpp"
#include "ttx/semantic/flows/copy.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Model;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;

static Validation::Harness Types = {.name = "Model::Type::Structure"_view};

struct Position {
  R64 x;
  R64 y;
};

struct Record {
  U8 tag;
  Position position;
  S32 samples[4];
};

TTX_DATA_RECORD(
    Position,
    TTX_DATA_MEMBER(Position, x),
    TTX_DATA_MEMBER(Position, y));
TTX_DATA_RECORD(
    Record,
    TTX_DATA_MEMBER(Record, tag),
    TTX_DATA_MEMBER(Record, position),
    TTX_DATA_MEMBER(Record, samples));

// This value includes alignment padding, a nested record and a compact range.
// Its Type supplies the prepared form directly. Copying observations never
// traverses a second Abstract inventory of the individual primitive fields.
PERIMORTEM_UNIT_TEST(Types, nested_plain_record) {
  const auto& form = Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<Record>::reference>::get_representation();
  const Type::Primitives::Structure provider(form);
  const auto type = Abstract::provide(provider);
  EXPECT(type.supports<Type::Policies::Plain>() == Binding::Status::Satisfied);
  EXPECT(
      type.supports<Type::Policies::Unsigned>() ==
      Binding::Status::Unsupported);

  const Record input(7, Position(1.5, -2.25), {1, -2, 3, -4});
  const Execution::Values::Literal<Record> value(type, input);
  Ttx::Semantic::Transport::Flow flow;
  ASSERT(
      flow.connect(
          decltype(flow)::reader(form), Abstract::provide(value).get_query()) ==
      decltype(flow)::Status::Success);

  Record output = {};
  const Ttx::Data::Form::Storage destination(
      ttx_storage{&form, reinterpret_cast<U8*>(&output), sizeof(output)});
  for (Count index = 0; index < 1000; ++index) {
    ASSERT(
        Ttx::Semantic::Flows::Copy::flow(flow, destination) ==
        Ttx::Data::Status::Success);
  }

  EXPECT_EQ(output.tag, input.tag);
  EXPECT_EQ(output.position.x, input.position.x);
  EXPECT_EQ(output.position.y, input.position.y);
  for (Count index = 0; index < 4; ++index) {
    EXPECT_EQ(output.samples[index], input.samples[index]);
  }

  const Execution::Assignments::Memory location(type, destination);
  location.assign(Abstract::provide(value))
      .visit(
          [&](Ttx::Data::Status status) {
            EXPECT(status == Ttx::Data::Status::Success);
          },
          [&](Binding::Failure) { EXPECT(False); });
}
