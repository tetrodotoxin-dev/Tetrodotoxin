// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"
#include "validation/unit_tests/ttx/concept/fixtures/subject.hpp"

#include <unistd.h>

#include "perimortem/system/library.hpp"

#include "ttx/concept/answers/constant.hpp"
#include "ttx/concept/answers/none.hpp"
#include "validation/unit_tests/ttx/concept/fixtures/observation.h"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Validation::ConceptTests;

static Validation::Harness Abstracts = {.name = "TTX::Abstract"_view};

struct Requirement {
  U8 family;
  auto get_data() const -> Core::View::Bytes {
    return Core::View::Bytes(&family, 1);
  }
};

PERIMORTEM_UNIT_TEST(Abstracts, c_observations) {
  observation_subject owner{
    &Binding::representation<Abstract>(),
    &Binding::representation<Answers::Constant>(), 0};
  const Abstract subject(observation_abstract(&owner));
  subject.get_query().bind<Abstract>().visit(
      [&](Abstract acquired) { EXPECT(acquired == subject); },
      [&](Binding::Failure) { EXPECT(False); });
  subject.bind<Answers::Constant>().visit(
      [](Answers::Constant) {}, [&](Binding::Failure) { EXPECT(False); });

  // Save the byte itself, since a new observation ends the first view's
  // retention promise. A Constant marker does not change that rule.
  const U8 first = subject.get_data().get_data()[0];
  const U8 second = subject.get_data().get_data()[0];
  EXPECT_EQ(first, U8(1));
  EXPECT_EQ(second, U8(2));
  EXPECT(subject.resolve().resolve() == subject);

  Count visited = 0;
  auto receive = [&](Core::View::Bytes route, Abstract value) {
    ++visited;
    EXPECT_EQ(route.get_size(), Count(3));
    EXPECT_EQ(route.get_data()[1], U8(0));
    EXPECT(subject.resolve_concept(route) == value);
  };
  subject.visit_concepts(Abstract::Visitor(receive));
  EXPECT_EQ(visited, Count(1));
  EXPECT(subject.resolve_concept("missing"_view) == Answers::None::get_none());
}

PERIMORTEM_UNIT_TEST(Abstracts, local_cast) {
  const Subject owner;
  const auto subject = Abstract::provide(owner);
  const auto local = subject.cast<Subject>();
  ASSERT(local);
  EXPECT(&*local == &owner);
  EXPECT_NOT(subject.cast<Requirement>());
  static_assert(__is_same(decltype(*local), const Subject&));
  static_assert(sizeof(Abstract) == sizeof(ttx_abstract));
}

PERIMORTEM_UNIT_TEST(Abstracts, foreign_same_type) {
  // Bazel places the fixture beside this executable. The library owner stays
  // alive through every borrowed observation, including the typed bind below.
  char path[4096];
  const auto length = readlink("/proc/self/exe", path, sizeof(path));
  ASSERT(length > 0 && length < static_cast<ssize_t>(sizeof(path)));
  Count end = length;
  while (end && path[end - 1] != '/') {
    --end;
  }
  const auto suffix = "libabstract_subject.so"_view;
  ASSERT(end + suffix.get_size() <= sizeof(path));
  Core::Data::copy(
      reinterpret_cast<U8*>(path + end), suffix.get_data(), suffix.get_size());
  Memory::Allocator::Arena errors;
  auto library = System::Library::open(
      Core::View::Bytes(
          reinterpret_cast<const U8*>(path), end + suffix.get_size()),
      errors);
  library.visit(
      [&](System::Library& module) {
        module.symbol("abstract_subject"_view, errors)
            .visit(
                [&](void* symbol) {
                  const auto open =
                      reinterpret_cast<ttx_abstract (*)()>(symbol);
                  const Abstract foreign(open());
                  const Subject local;
                  EXPECT_NOT(foreign.cast<Subject>());
                  EXPECT(foreign.get_data() == local.get_data());
                  EXPECT(foreign.resolve() == foreign);
                  foreign.get_query().bind<Abstract>().visit(
                      [&](Abstract acquired) { EXPECT(acquired == foreign); },
                      [&](Binding::Failure) { EXPECT(False); });
                },
                [&](Core::View::Bytes) { EXPECT(False); });
      },
      [&](Core::View::Bytes) { EXPECT(False); });
}

// A provider implements the UUID question directly. Acquiring a richer C++
// view over its publication must not republish the view as a second subject or
// mistake consumer forwarding methods for provider operations.
PERIMORTEM_UNIT_TEST(Abstracts, native_supports) {
  struct Owner {
    auto get_data() const -> Core::View::Bytes { return "provider"_view; }
    auto supports(System::Uuid id) const -> Binding::Status {
      return id == Answers::Constant::contract_id ? Binding::Status::Satisfied
                                                  : Binding::Status::Rejected;
    }
  } owner;
  const auto subject = Abstract::provide(owner);
  struct View : Abstract {
    explicit View(Abstract subject) : Abstract(subject) {}
  } view(subject);
  EXPECT(Abstract::provide(view) == subject);
  EXPECT(view.supports<Answers::Constant>() == Binding::Status::Satisfied);
  EXPECT(view.supports<Answers::None>() == Binding::Status::Rejected);
  EXPECT(view.cast<Owner>());
  EXPECT_NOT(view.cast<View>());
}
