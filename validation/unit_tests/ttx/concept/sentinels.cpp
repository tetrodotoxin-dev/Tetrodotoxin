// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "ttx/concept/answers/constant.hpp"
#include "ttx/concept/answers/none.hpp"
#include "ttx/concept/answers/unknown.hpp"

using namespace Perimortem;
using namespace Ttx;
using namespace Validation;

static Harness Sentinels = {.name = "TTX::Sentinels"_view};

static_assert(__is_empty(Concept::Answers::None));
static_assert(__is_empty(Concept::Answers::Unknown));
static_assert(__is_empty(Concept::Answers::Constant));

// Keep the outer record and every table slot the same size. Only get_data's
// result differs, so rejection proves that agreement follows the typed table
// pointer rather than treating the Abstract as two opaque pointer slots.
struct WrongAbstractOperations {
  decltype(ttx_abstract_ops::bind) bind;
  U64 (*get_data)(const void*);
  decltype(ttx_abstract_ops::resolve) resolve;
  decltype(ttx_abstract_ops::resolve_concept) resolve_concept;
  decltype(ttx_abstract_ops::visit_concepts) visit_concepts;
  decltype(ttx_abstract_ops::satisfies) satisfies;
};

struct WrongAbstract {
  const void* source;
  const WrongAbstractOperations* operations;
};

TTX_DATA_RECORD(
    WrongAbstractOperations,
    TTX_DATA_MEMBER(WrongAbstractOperations, bind),
    TTX_DATA_MEMBER(WrongAbstractOperations, get_data),
    TTX_DATA_MEMBER(WrongAbstractOperations, resolve),
    TTX_DATA_MEMBER(WrongAbstractOperations, resolve_concept),
    TTX_DATA_MEMBER(WrongAbstractOperations, visit_concepts),
    TTX_DATA_MEMBER(WrongAbstractOperations, satisfies));
TTX_DATA_RECORD(
    WrongAbstract,
    TTX_DATA_MEMBER(WrongAbstract, source),
    TTX_DATA_MEMBER(WrongAbstract, operations));

PERIMORTEM_UNIT_TEST(Sentinels, nested_table_abi) {
  static_assert(sizeof(WrongAbstract) == sizeof(ttx_abstract));
  static_assert(sizeof(WrongAbstractOperations) == sizeof(ttx_abstract_ops));
  const auto& form = Data::Form::Compiled<
      Data::Form::Native<WrongAbstract>::reference>::get_representation();
  WrongAbstract output = {};
  const Data::Form::Storage target(
      ttx_storage{&form, reinterpret_cast<U8*>(&output), sizeof(output)});
  const auto query = Concept::Answers::None::get_none().get_query();
  EXPECT(
      query.bind(Concept::Abstract::contract_id, target) ==
      Semantic::Negotiation::Binding::Status::Rejected);
  EXPECT(output.source == nullptr && output.operations == nullptr);
}

// A marker uses Empty storage and transfers no artificial receiver/table pair.
// Requesting an operational form under that marker must instead be rejected.
PERIMORTEM_UNIT_TEST(Sentinels, marker_carrier) {
  const auto none = ttx_none();
  const auto& empty =
      Semantic::Negotiation::Binding::representation<Concept::Answers::None>();
  const ttx_storage output{&empty, nullptr, 0};
  EXPECT_EQ(
      none.operations->bind(
          none.source, Concept::Answers::None::contract_id, output),
      TTX_BINDING_SATISFIED);

  const auto unknown = ttx_unknown();
  EXPECT_EQ(
      unknown.operations->bind(
          unknown.source, Concept::Answers::Unknown::contract_id, output),
      TTX_BINDING_SATISFIED);
  Concept::Abstract(none).bind<Concept::Answers::None>().visit(
      [&](Concept::Answers::None) {},
      [&](Semantic::Negotiation::Binding::Failure) { EXPECT(False); });
  Concept::Abstract(none).bind<Concept::Answers::Constant>().visit(
      [&](Concept::Answers::Constant) {},
      [&](Semantic::Negotiation::Binding::Failure) { EXPECT(False); });
}

// Marker negotiation does not replace navigation. Abstract still binds its
// own table, and Unknown preserves Pending for questions it cannot settle.
PERIMORTEM_UNIT_TEST(Sentinels, navigation_and_pending) {
  const auto unknown = Concept::Abstract(ttx_unknown());
  unknown.get_query().bind<Concept::Abstract>().visit(
      [&](Concept::Abstract root) {
        EXPECT(root.get_identity() == unknown.get_identity());
        EXPECT(root.get_data() == "Unknown"_view);
      },
      [&](Semantic::Negotiation::Binding::Failure) { EXPECT(False); });

  unknown.bind<Concept::Answers::Constant>().visit(
      [&](Concept::Answers::Constant) { EXPECT(False); },
      [&](Semantic::Negotiation::Binding::Failure failure) {
        EXPECT(failure == Semantic::Negotiation::Binding::Failure::Pending);
      });
  Concept::Answers::None::get_none().bind<Concept::Answers::Unknown>().visit(
      [&](Concept::Answers::Unknown) { EXPECT(False); },
      [&](Semantic::Negotiation::Binding::Failure failure) {
        EXPECT(failure == Semantic::Negotiation::Binding::Failure::Unsupported);
      });
}

// The same empty C answer is sufficient for a marker and insufficient for an
// operational contract. Keeping admission typed prevents the marker exception
// from weakening the table requirement used by all ordinary interfaces.
PERIMORTEM_UNIT_TEST(Sentinels, empty_table_admission) {
  Semantic::Negotiation::Query query(
      {nullptr,
       [](const void*, perimortem_uuid,
          ttx_storage output) -> ttx_binding_status {
         (void)output;
         return TTX_BINDING_SATISFIED;
       }});
  query.bind<Concept::Answers::Constant>().visit(
      [&](Concept::Answers::Constant) {},
      [&](Semantic::Negotiation::Binding::Failure) { EXPECT(False); });
  query.bind<Concept::Abstract>().visit(
      [&](Concept::Abstract) { EXPECT(False); },
      [&](Semantic::Negotiation::Binding::Failure failure) {
        EXPECT(failure == Semantic::Negotiation::Binding::Failure::Rejected);
      });
}

// A native owner need not inherit an Abstract or reproduce its virtual base.
// The published view must nevertheless retain the encountered child policy
// through both navigation paths and negotiate the same marker as a C owner.
PERIMORTEM_UNIT_TEST(Sentinels, native_publication) {
  struct Leaf {
    auto get_data() const -> Core::View::Bytes { return "value"_view; }
    auto bind_interface(System::Uuid id, Data::Form::Storage requested) const
        -> Semantic::Negotiation::Binding::Status {
      if (id == Concept::Answers::Constant::contract_id) {
        return Semantic::Negotiation::Binding::marker(requested);
      }

      return Semantic::Negotiation::Binding::Status::Rejected;
    }
  } leaf;
  struct Parent {
    const Leaf& leaf;
    auto get_data() const -> Core::View::Bytes { return "parent"_view; }
    auto resolve_concept(Core::View::Bytes name) const -> Concept::Abstract {
      return name == leaf.get_data() ? Concept::Abstract::provide(leaf)
                                     : Concept::Answers::None::get_none();
    }
    auto visit_concepts(Concept::Abstract::Visitor visitor) const -> void {
      visitor(leaf.get_data(), Concept::Abstract::provide(leaf));
    }
  } parent(leaf);

  const auto root = Concept::Abstract::provide(parent);
  const auto child = root.resolve_concept("value"_view);
  child.bind<Concept::Answers::Constant>().visit(
      [](Concept::Answers::Constant) {},
      [&](Semantic::Negotiation::Binding::Failure) { EXPECT(False); });
  child.bind<Concept::Answers::Unknown>().visit(
      [&](Concept::Answers::Unknown) { EXPECT(False); },
      [&](Semantic::Negotiation::Binding::Failure failure) {
        EXPECT(failure == Semantic::Negotiation::Binding::Failure::Rejected);
      });

  Count visited = 0;
  auto visitor = [&](Core::View::Bytes name, Concept::Abstract subject) {
    EXPECT(name == "value"_view);
    EXPECT(subject.get_identity() == child.get_identity());
    ++visited;
  };
  root.visit_concepts(Concept::Abstract::Visitor(visitor));
  EXPECT_EQ(visited, Count(1));
  child.resolve_concept("absent"_view)
      .bind<Concept::Answers::None>()
      .visit(
          [](Concept::Answers::None) {},
          [&](Semantic::Negotiation::Binding::Failure) { EXPECT(False); });
}
