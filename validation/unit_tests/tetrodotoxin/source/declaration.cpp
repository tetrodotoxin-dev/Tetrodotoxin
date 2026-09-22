// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/source/declaration.h"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/source/alias.hpp"
#include "tetrodotoxin/source/contents/memory.hpp"
#include "tetrodotoxin/source/declaration.hpp"
#include "tetrodotoxin/source/policies/authored.hpp"
#include "ttx/concept/answers/constant.hpp"
#include "ttx/semantic/negotiation/query.hpp"

using namespace Ttx::Concept;
using namespace Ttx::Concept::Answers;
using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Ttx::Semantic::Negotiation;

static Validation::Harness SourceDeclarations = {
  .name = "Source::Declaration"_view,
};

PERIMORTEM_UNIT_TEST(SourceDeclarations, native_provenance) {
  // Coordinates are data, not lexer Tokens. This observation has no tokenizer
  // and needs no resident terabyte buffer to publish a location within it.
  struct Observation {
    auto get_data() const -> View::Bytes { return {}; }
  } observation;
  const auto source = Abstract::provide(observation);
  const U64 offset = (U64(1) << 32) + 7;
  Policies::Authored policy(
      None::get_none(), Anchor(source, Range(offset, 25), Range(offset, 0)),
      "value"_view);

  Abstract::provide(policy).bind<Declaration>().visit(
      [&](Declaration declaration) {
        const auto anchor = declaration.get_anchor();
        ASSERT(anchor);
        EXPECT(anchor->get_source() == source);
        EXPECT_EQ(anchor->get_extent().get_offset(), offset);
        ASSERT(anchor->get_focus());
        EXPECT_EQ(anchor->get_focus()->get_size(), U64(0));

        // A zero length caret and an absent focus are different observations.
        // Updating authored evidence does not complete or freeze the policy.
        policy.set_anchor(Anchor(source, Range(offset, 25)));
        EXPECT_NOT(declaration.get_anchor()->get_focus());
        EXPECT_EQ(declaration.get_anchor()->get_extent().get_size(), U64(25));
      },
      [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(SourceDeclarations, foreign_provenance) {
  struct Observation {
    auto get_data() const -> View::Bytes { return {}; }
  } observation;
  const auto source = Abstract::provide(observation);
  const Anchor authored(source, Range(U64(1) << 33, 4));
  const Query query(source_declaration_fixture(&authored));
  EXPECT(query.supports<Declaration>() == Binding::Status::Satisfied);
  query.bind<Declaration>().visit(
      [&](Declaration declaration) {
        const auto anchor = declaration.get_anchor();
        ASSERT(anchor);
        EXPECT(anchor->get_source() == source);
        EXPECT_EQ(anchor->get_extent().get_offset(), U64(1) << 33);
        EXPECT_NOT(anchor->get_focus());
      },
      [&](Binding::Failure) { EXPECT(False); });

  // Absence is a successful declaration binding with no authored location.
  // The C operation must leave an existing output unchanged on that path.
  const Query synthetic(source_declaration_fixture(nullptr));
  synthetic.bind<Declaration>().visit(
      [&](Declaration declaration) { EXPECT_NOT(declaration.get_anchor()); },
      [&](Binding::Failure) { EXPECT(False); });
  Declaration::Api api = {};
  const auto& form = Binding::representation<Declaration>();
  const Ttx::Data::Form::Storage destination(
      ttx_storage{&form, reinterpret_cast<U8*>(&api), sizeof(api)});
  ASSERT(
      synthetic.bind(Declaration::contract_id, destination) ==
      Binding::Status::Satisfied);
  auto untouched = authored;
  EXPECT_EQ(api.get_anchor(api.source, &untouched), U8(0));
  EXPECT(untouched.get_source() == source);
  EXPECT_EQ(untouched.get_extent().get_offset(), U64(1) << 33);
}

struct WrongDeclaration {
  const void* source;
  U64 (*get_anchor)(const void*, tetrodotoxin_source_anchor*);
};

TTX_DATA_RECORD(
    WrongDeclaration,
    TTX_DATA_MEMBER(WrongDeclaration, source),
    TTX_DATA_MEMBER(WrongDeclaration, get_anchor));

PERIMORTEM_UNIT_TEST(SourceDeclarations, signature_rejection) {
  // Equal record sizes are insufficient: this consumer expects a different
  // return carrier from the same operation UUID. Binding must reject it before
  // copying any API bytes into the consumer's storage.
  const Query query(source_declaration_fixture(nullptr));
  WrongDeclaration output = {&query, nullptr};
  const auto& form = Ttx::Data::Form::Compiled<Ttx::Data::Form::Native<
      WrongDeclaration>::reference>::get_representation();
  const Ttx::Data::Form::Storage destination(
      ttx_storage{&form, reinterpret_cast<U8*>(&output), sizeof(output)});
  EXPECT(
      query.bind(Declaration::contract_id, destination) ==
      Binding::Status::Rejected);
  EXPECT(output.source == &query);
  EXPECT(output.get_anchor == nullptr);
}

class RestrictedSubject {
 public:
  auto get_data() const -> View::Bytes { return "restricted"_view; }

  auto bind_interface(Perimortem::System::Uuid, Ttx::Data::Form::Storage) const
      -> Binding::Status {
    return Binding::Status::Rejected;
  }
};

PERIMORTEM_UNIT_TEST(SourceDeclarations, policy_forwarding) {
  const RestrictedSubject restricted;
  Alias alias;
  ASSERT(alias.commit(Abstract::provide(restricted)));

  // The retained Alias asks the encountered policy. Neither the native bridge
  // nor its C query resolves through that policy to obtain a different answer.
  Abstract::provide(alias).bind<Declaration>().visit(
      [&](Declaration) { EXPECT(False); },
      [&](Binding::Failure failure) {
        EXPECT(failure == Binding::Failure::Rejected);
      });
}

PERIMORTEM_UNIT_TEST(SourceDeclarations, sentinel_markers) {
  None::get_none().bind<Ttx::Concept::Answers::None>().visit(
      [](Ttx::Concept::Answers::None) {},
      [&](Binding::Failure) { EXPECT(False); });
  None::get_none().bind<Ttx::Concept::Answers::Constant>().visit(
      [](Ttx::Concept::Answers::Constant) {},
      [&](Binding::Failure) { EXPECT(False); });
  Unknown::get_unknown().bind<Ttx::Concept::Answers::Unknown>().visit(
      [](Ttx::Concept::Answers::Unknown) {},
      [&](Binding::Failure) { EXPECT(False); });

  Unknown::get_unknown().bind<Declaration>().visit(
      [&](Declaration) { EXPECT(False); },
      [&](Binding::Failure failure) {
        EXPECT(failure == Binding::Failure::Pending);
      });
}

PERIMORTEM_UNIT_TEST(SourceDeclarations, authored_policy) {
  const RestrictedSubject restricted;
  const Anchor anchor(Abstract::provide(restricted), Range(0, 10), Range(3, 2));
  Policies::Authored policy(Abstract::provide(restricted), anchor, "name"_view);
  const auto view = Abstract::provide(policy);

  // Provenance can be added without acquiring the wrapped subject's other
  // capabilities. Rejection remains visible through the encountered policy.
  EXPECT(view.supports<Declaration>() == Binding::Status::Satisfied);
  view.bind<Content>().visit(
      [&](Content) { EXPECT(False); },
      [&](Binding::Failure error) {
        EXPECT(error == Binding::Failure::Rejected);
      });
  view.bind<Declaration>().visit(
      [&](Declaration declaration) {
        ASSERT(declaration.get_anchor());
        EXPECT_EQ(declaration.get_anchor()->get_focus()->get_offset(), U64(3));
      },
      [&](Binding::Failure) { EXPECT(False); });
}
