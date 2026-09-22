// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/semantic/realization/simulacra.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/ttx/semantic/measurement.hpp"

#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

#include "validation/unit_tests/ttx/semantic/fixtures/interface_provider.h"

using namespace Perimortem;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Realization;
using Ttx::Data::Form::Compiled;
using Ttx::Data::Form::Native;
using Ttx::Data::Form::Storage;

TTX_DATA_RECORD(
    counter_api,
    TTX_DATA_MEMBER(counter_api, receiver),
    TTX_DATA_MEMBER(counter_api, add),
    TTX_DATA_MEMBER(counter_api, read));

static Validation::Harness TtxSimulacra = {.name = "TTX::Simulacra"_view};

// This API is three words, with the functions themselves in the transferred
// record. Its facade owns those words while borrowing the foreign receiver.
class Counter {
 public:
  static constexpr System::Uuid contract_id{COUNTER_ID_HIGH, COUNTER_ID_LOW};
  using Api = counter_api;
  static auto accept(Api api) -> Bool { return api.add && api.read; }

  explicit Counter(Api api) : api(api) {}
  auto add(U64 value) const -> U64 { return api.add(api.receiver, value); }
  auto read() const -> U64 { return api.read(api.receiver); }

 private:
  Api api;
};

class Foreign {
 public:
  Foreign() {
    char path[4096];
    const auto size = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (size <= 0) {
      return;
    }

    path[size] = 0;
    auto* name = strrchr(path, '/');
    if (!name ||
        sizeof(path) - (name + 1 - path) < sizeof("libinterface_provider.so")) {
      return;
    }

    memcpy(
        name + 1, "libinterface_provider.so",
        sizeof("libinterface_provider.so"));
    module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (module) {
      const auto open = reinterpret_cast<decltype(&interface_provider_open)>(
          dlsym(module, "interface_provider_open"));
      if (open) {
        api = open(&ttx_representation_compile);
      }
    }
  }

  ~Foreign() {
    if (module) {
      dlclose(module);
    }
  }

  counter_fixture api = {};

 private:
  void* module = nullptr;
};

// The semantic promise survives API drift. The first observation has no
// destination and cannot transfer a table. Binding still rejects a caller's
// incompatible format and leaves its bytes untouched.
PERIMORTEM_UNIT_TEST(TtxSimulacra, support_without_abi) {
  Foreign foreign;
  const Query query(foreign.api.query);
  ASSERT(query.is_set());

  Validation::FlowTests::Measurement measurement;
  EXPECT(query.supports<Counter>() == Binding::Status::Satisfied);
  measurement.stop();

  EXPECT_EQ(measurement.get_allocations(), Count(0));
  EXPECT_EQ(measurement.get_copies(), Count(0));
  EXPECT_EQ(foreign.api.statistics().queries, U64(0));

  U64 output = 0x1234;
  const auto& form = Compiled<Native<U64>::reference>::get_representation();
  const Storage target(
      ttx_storage{&form, reinterpret_cast<U8*>(&output), sizeof(output)});
  EXPECT(query.bind(Counter::contract_id, target) == Binding::Status::Rejected);
  EXPECT_EQ(output, U64(0x1234));
  EXPECT(query.supports<Counter>() == Binding::Status::Satisfied);
  EXPECT_EQ(foreign.api.statistics().queries, U64(1));

  // The C entry answers the same question without any C++ descriptor machinery.
  EXPECT_EQ(
      foreign.api.query.supports(
          foreign.api.query.source, Counter::contract_id),
      TTX_BINDING_SATISFIED);
}

// A refused or unsettled property cannot become false just because the caller
// wanted a predicate. Retaining statuses keeps a later policy from bypassing
// this answer or treating provisional absence as a completed fact.
PERIMORTEM_UNIT_TEST(TtxSimulacra, support_outcomes) {
  Foreign foreign;
  const Query query(foreign.api.query);
  ASSERT(query.is_set());

  const Binding::Status statuses[] = {
    Binding::Status::Unsupported, Binding::Status::Pending,
    Binding::Status::Rejected};
  for (const auto status : statuses) {
    foreign.api.reset(static_cast<ttx_binding_status>(status), 0, 0);
    EXPECT(query.supports<Counter>() == status);
    EXPECT_EQ(foreign.api.statistics().queries, U64(0));
  }

  foreign.api.reset(99, 0, 0);
  EXPECT(query.supports<Counter>() == Binding::Status::Rejected);
  EXPECT(Query().supports<Counter>() == Binding::Status::Rejected);
}

// The provider compiles its own C Schema at module opening. C++ derives this
// side's description from counter_api. One checked bind copies the entire API;
// all later calls use the acquired functions without metadata or allocation.
PERIMORTEM_UNIT_TEST(TtxSimulacra, retained_c_calls) {
  Foreign foreign;
  ASSERT(foreign.api.query.bind);
  Simulacra::fulfill<Counter>(Query(foreign.api.query))
      .visit(
          [&](const Counter& counter) {
            Validation::FlowTests::Measurement measurement;
            U64 answer = 0;
            for (U64 i = 0; i < 1000000; ++i) {
              answer = counter.add(1);
            }
            measurement.stop();

            EXPECT_EQ(answer, U64(1000000));
            EXPECT_EQ(counter.read(), answer);
            EXPECT_EQ(measurement.get_allocations(), Count(0));
            EXPECT_EQ(measurement.get_copies(), Count(0));
          },
          [&](Binding::Failure) { EXPECT(False); });

  EXPECT_EQ(foreign.api.statistics().queries, U64(1));
  EXPECT_EQ(foreign.api.statistics().calls, U64(1000001));
}

PERIMORTEM_UNIT_TEST(TtxSimulacra, refusal_boundaries) {
  Foreign foreign;
  ASSERT(foreign.api.query.bind);
  const ttx_binding_status statuses[] = {
    TTX_BINDING_UNSUPPORTED, TTX_BINDING_PENDING, TTX_BINDING_REJECTED, 99};
  for (const auto status : statuses) {
    foreign.api.reset(status, 0, 0);
    Query(foreign.api.query)
        .bind<Counter>()
        .visit(
            [&](const Counter&) { EXPECT(False); },
            [&](Binding::Failure error) {
              EXPECT_EQ(
                  static_cast<U8>(error),
                  status == 99 ? TTX_BINDING_REJECTED : status);
            });
    EXPECT_EQ(foreign.api.statistics().queries, U64(1));
    EXPECT_EQ(foreign.api.statistics().calls, U64(0));
  }

  // The generic exchange accepts values according to their contract. Counter
  // requires both functions, so a provider that leaves the output empty cannot
  // produce its typed view. A null opaque receiver remains valid.
  foreign.api.reset(TTX_BINDING_SATISFIED, 1, 0);
  Query(foreign.api.query)
      .bind<Counter>()
      .visit(
          [&](const Counter&) { EXPECT(False); },
          [&](Binding::Failure error) {
            EXPECT(error == Binding::Failure::Rejected);
          });
  foreign.api.reset(TTX_BINDING_SATISFIED, 0, 1);
  Query(foreign.api.query)
      .bind<Counter>()
      .visit(
          [&](const Counter& counter) { EXPECT_EQ(counter.add(17), U64(17)); },
          [&](Binding::Failure) { EXPECT(False); });
}

struct WrongResult {
  const void* receiver;
  R64 (*add)(const void*, U64);
  U64 (*read)(const void*);
};
struct WrongArguments {
  const void* receiver;
  U64 (*add)(U64);
  U64 (*read)(const void*);
};
struct WrongAbi {
  const void* receiver;
  U64 (*add)(const void*, U64, ...);
  U64 (*read)(const void*);
};
struct WrongOrder {
  const void* receiver;
  U64 (*read)(const void*);
  U64 (*add)(const void*, U64);
};
TTX_DATA_RECORD(
    WrongResult,
    TTX_DATA_MEMBER(WrongResult, receiver),
    TTX_DATA_MEMBER(WrongResult, add),
    TTX_DATA_MEMBER(WrongResult, read));
TTX_DATA_RECORD(
    WrongArguments,
    TTX_DATA_MEMBER(WrongArguments, receiver),
    TTX_DATA_MEMBER(WrongArguments, add),
    TTX_DATA_MEMBER(WrongArguments, read));
TTX_DATA_RECORD(
    WrongAbi,
    TTX_DATA_MEMBER(WrongAbi, receiver),
    TTX_DATA_MEMBER(WrongAbi, add),
    TTX_DATA_MEMBER(WrongAbi, read));
TTX_DATA_RECORD(
    WrongOrder,
    TTX_DATA_MEMBER(WrongOrder, receiver),
    TTX_DATA_MEMBER(WrongOrder, read),
    TTX_DATA_MEMBER(WrongOrder, add));

// Every record has the same byte extent and requests the same semantic UUID.
// Only the callable descriptors differ. Rejection must precede copying or
// invocation, including when the disagreement is solely the calling ABI.
PERIMORTEM_UNIT_TEST(TtxSimulacra, mismatched_callables) {
  Foreign foreign;
  ASSERT(foreign.api.query.bind);
  const Ttx::Data::Form::Representation* forms[] = {
    &Compiled<Native<WrongResult>::reference>::get_representation(),
    &Compiled<Native<WrongArguments>::reference>::get_representation(),
    &Compiled<Native<WrongAbi>::reference>::get_representation(),
    &Compiled<Native<WrongOrder>::reference>::get_representation()};
  alignas(counter_api) U8 output[sizeof(counter_api)];
  U8 expected[sizeof(output)];
  memset(expected, 0xa5, sizeof(expected));
  for (const auto* form : forms) {
    memcpy(output, expected, sizeof(output));
    const auto status =
        Query(foreign.api.query)
            .bind(
                Counter::contract_id,
                Storage(ttx_storage{form, output, sizeof(output)}));
    EXPECT(status == Binding::Status::Rejected);
    EXPECT(memcmp(output, expected, sizeof(output)) == 0);
  }

  EXPECT_EQ(foreign.api.statistics().calls, U64(0));
}

class Local {
 public:
  Local(Query fallback, Binding::Status status)
      : fallback(fallback), status(status) {}
  template <typename Contract>
  auto fulfill_native() const -> Utility::Result<Contract, Binding::Failure> {
    if (status != Binding::Status::Satisfied) {
      return static_cast<Binding::Failure>(status);
    }
    if constexpr (__is_same(Contract, Counter)) {
      const counter_api api{
        nullptr, [](const void*, U64 amount) -> U64 { return amount; },
        [](const void*) -> U64 { return 0; }};
      return Counter(api);
    }
    return Binding::Failure::Unsupported;
  }
  auto get_query() const -> Query { return fallback; }

 private:
  Query fallback;
  Binding::Status status;
};

PERIMORTEM_UNIT_TEST(TtxSimulacra, native_policy) {
  Foreign foreign;
  ASSERT(foreign.api.query.bind);
  const Binding::Status statuses[] = {
    Binding::Status::Satisfied, Binding::Status::Unsupported,
    Binding::Status::Rejected, Binding::Status::Pending};
  for (auto status : statuses) {
    foreign.api.reset(TTX_BINDING_SATISFIED, 0, 0);
    const Local native(Query(foreign.api.query), status);
    Simulacra::fulfill<Counter>(native).visit(
        [&](const Counter& counter) {
          EXPECT(
              status == Binding::Status::Satisfied ||
              status == Binding::Status::Unsupported);
          EXPECT_EQ(counter.add(9), U64(9));
        },
        [&](Binding::Failure failure) {
          EXPECT_EQ(static_cast<U8>(failure), static_cast<U8>(status));
        });
    EXPECT_EQ(
        foreign.api.statistics().queries,
        status == Binding::Status::Unsupported ? U64(1) : U64(0));
  }
}
