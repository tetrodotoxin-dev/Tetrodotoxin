// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "tetrodotoxin/source/alias.hpp"
#include "ttx/concept/answers/constant.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;

static Validation::Harness Aliases = {.name = "Source::Alias"_view};

struct AliasSubject {
  auto get_data() const -> View::Bytes { return "subject"_view; }
  auto supports(Perimortem::System::Uuid id) const -> Binding::Status {
    return id == Answers::Constant::contract_id ? Binding::Status::Satisfied
                                                : Binding::Status::Unsupported;
  }
};

PERIMORTEM_UNIT_TEST(Aliases, commitment) {
  AliasSubject owner;
  AliasSubject other;
  const auto subject = Abstract::provide(owner);
  Tetrodotoxin::Source::Alias alias;

  // A committed Alias observes its referent through the same C surface as
  // every other consumer. No native subject reference is retained by the edge.
  EXPECT_NOT(alias.commit(Answers::None::get_none()));
  EXPECT(alias.commit(subject));
  EXPECT(alias.commit(subject));
  EXPECT_NOT(alias.commit(Abstract::provide(other)));
  EXPECT(alias.resolve() == subject);
  EXPECT(alias.resolve().resolve() == subject);
  EXPECT(alias.get_data() == "subject"_view);
  EXPECT(Abstract::provide(alias).supports<Answers::Constant>() == Binding::Status::Satisfied);

  Tetrodotoxin::Source::Alias nested;
  EXPECT(nested.commit(Abstract::provide(alias)));
  EXPECT(nested.resolve() == subject);
}
