// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/ttx/semantic/fixtures.hpp"

using namespace Validation::FlowTests;

// The module entry is the prearranged C bootstrap. It supplies a writer for
// the Query record, while the host's reader permits only Direct or Shared.
// The agreed C representation grants the cast that imports its bind thunk.
// That imported Query then participates in ordinary protocol negotiation.
PERIMORTEM_UNIT_TEST(TtxFlow, bootstrap_bind) {
  Preparation prepare;
  const auto& four = prepare(four_schema);

  Module module;
  ASSERT(module.is_set());

  // Derive the expected callable form from the actual C declaration. The C
  // provider authors the same form independently, including bind's signature.
  const auto& query_schema = Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<ttx_semantic_query>::reference>::get_representation();
  Validation::FlowTests::Reader receiver{query_schema, PROVIDES_DIRECT | PROVIDES_SHARED};

  Flow bootstrap;
  ASSERT(
      bootstrap.connect(receiver.query(), module.bootstrap_writer()) ==
      Flow::Status::Success);
  EXPECT(bootstrap.get_protocol() == Protocol::Direct);

  const auto imported = module.import_query(bootstrap);
  Validation::FlowTests::Reader data{four};
  Flow flow;
  ASSERT(flow.connect(data.query(), imported) == Flow::Status::Success);

  U32 target[4] = {};
  ASSERT(Copy::flow(flow, storage(four, target)) == Status::Success);
  EXPECT_EQ(target[3], U32(4));
}
