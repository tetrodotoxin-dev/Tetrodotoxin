// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/semantic/simulacra.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/ttx/semantic/measurement.hpp"

#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

#include "ttx/data/form/compiled.hpp"
#include "validation/unit_tests/ttx/semantic/fixtures/thunk_provider.h"

using namespace Perimortem;
using namespace Ttx::Semantic;
using Ttx::Data::Form::Schema;

static Validation::Harness TtxSimulacra = {.name = "TTX::Simulacra"_view};

// One contract, one C table and one typed facade. Signatures belong to this
// UUID agreement; Data's pointer field describes only the physical table.
class Counter {
 public:
  static constexpr System::Uuid contract_id{
    COUNTER_ID_HIGH,
    COUNTER_ID_LOW,
  };
  static constexpr auto convention = Thunk::Convention::SystemVAMD64;
  using Operations = counter_operations;

  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    static constexpr auto pointer = Schema::primitive(Schema::Value::Pointer);
    static constexpr Schema::Position position(pointer, 0);
    static constexpr auto schema = Schema::composite({&position, 1}, 8, 8);
    return Ttx::Data::Form::Compiled<schema>::get_representation();
  }

  class Handle : public Bound<Operations> {
   public:
    using Bound::Bound;
    auto add(U64 amount) const -> U64 { return operations.add(source, amount); }
  };
};

// The module scope encloses all borrowed interfaces. Closing it earlier would
// invalidate executable pointers even if the returned receiver were null.
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
        sizeof(path) - (name + 1 - path) < sizeof("libthunk_provider.so")) {
      return;
    }

    memcpy(name + 1, "libthunk_provider.so", sizeof("libthunk_provider.so"));
    module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (module) {
      const auto open = reinterpret_cast<decltype(&thunk_provider_open)>(
          dlsym(module, "thunk_provider_open"));
      if (open) {
        api = open(&Counter::get_representation());
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

PERIMORTEM_UNIT_TEST(TtxSimulacra, retained_c_calls) {
  Foreign foreign;
  ASSERT(foreign.api.query.bind != nullptr);
  auto acquired = Simulacra::fulfill<Counter>(Query(foreign.api.query));

  acquired.visit(
      [&](const Counter::Handle& counter) {
        // All negotiation ends before this loop. The provider applies an
        // interior receiver that C++ cannot inspect; each call is one typed
        // table dispatch with no TTX allocation or metadata observation.
        Validation::FlowTests::Measurement measurement;
        U64 answer = 0;
        for (U64 i = 0; i < 1000000; ++i) {
          answer = counter.add(1);
        }

        measurement.stop();
        EXPECT_EQ(answer, U64(1000000));
        EXPECT_EQ(measurement.get_allocations(), Count(0));
        EXPECT_EQ(measurement.get_copies(), Count(0));
      },
      [&](Binding::Failure) { EXPECT(False); });

  const auto counts = foreign.api.statistics();
  EXPECT_EQ(counts.queries, U64(1));
  EXPECT_EQ(counts.fulfillments, U64(1));
  EXPECT_EQ(counts.calls, U64(1000000));
}

PERIMORTEM_UNIT_TEST(TtxSimulacra, refusal_boundaries) {
  Foreign foreign;
  ASSERT(foreign.api.query.bind != nullptr);

  // Each failure stops at the boundary that produced it. A malformed success
  // must not reuse a previous pair, and an unknown status is a rejection.
  const ttx_binding_status failures[] = {
    TTX_BINDING_UNSUPPORTED, TTX_BINDING_PENDING, TTX_BINDING_REJECTED, 99};
  for (const auto status : failures) {
    for (U32 stage = 0; stage < 2; ++stage) {
      foreign.api.reset(
          stage == 0 ? status : TTX_BINDING_SATISFIED,
          stage == 1 ? status : TTX_BINDING_SATISFIED, 0, 0);
      Simulacra::fulfill<Counter>(Query(foreign.api.query))
          .visit(
              [&](const Counter::Handle&) { EXPECT(False); },
              [&](Binding::Failure error) {
                EXPECT(
                    static_cast<U8>(error) ==
                    (status == 99 ? TTX_BINDING_REJECTED : status));
              });
      EXPECT_EQ(foreign.api.statistics().fulfillments, U64(stage));
      EXPECT_EQ(foreign.api.statistics().calls, U64(0));
    }
  }

  foreign.api.reset(TTX_BINDING_SATISFIED, TTX_BINDING_SATISFIED, 1, 0);
  Simulacra::fulfill<Counter>(Query(foreign.api.query))
      .visit(
          [&](const Counter::Handle&) { EXPECT(False); },
          [&](Binding::Failure error) {
            EXPECT(error == Binding::Failure::Rejected);
          });
}

PERIMORTEM_UNIT_TEST(TtxSimulacra, realization_agreement) {
  Foreign foreign;
  ASSERT(foreign.api.query.bind != nullptr);
  const auto query = Query(foreign.api.query);
  const auto reject = [&](auto acquired, Binding::Failure expected) {
    acquired.visit(
        [&](const Binding&) { EXPECT(False); },
        [&](Binding::Failure error) { EXPECT(error == expected); });
  };

  reject(
      Simulacra::fulfill(
          query, System::Uuid(1, 2), Counter::convention,
          Counter::get_representation()),
      Binding::Failure::Unsupported);
  reject(
      Simulacra::fulfill(
          query, Counter::contract_id, Thunk::Convention(77),
          Counter::get_representation()),
      Binding::Failure::Unsupported);

  static constexpr auto scalar = Schema::primitive(Schema::Value::U32);
  reject(
      Simulacra::fulfill(
          query, Counter::contract_id, Counter::convention,
          Ttx::Data::Form::Compiled<scalar>::get_representation()),
      Binding::Failure::Rejected);

  // A null receiver is a valid foreign realization, not a native-mode tag.
  foreign.api.reset(TTX_BINDING_SATISFIED, TTX_BINDING_SATISFIED, 0, 1);
  Simulacra::fulfill<Counter>(query).visit(
      [&](const Counter::Handle& counter) {
        EXPECT_EQ(counter.add(17), U64(17));
      },
      [&](Binding::Failure) { EXPECT(False); });
}

class Native {
 public:
  Native(Query fallback, Binding::Status status)
      : fallback(fallback), status(status) {}

  template <typename Contract>
  auto fulfill_native() const
      -> Utility::Result<typename Contract::Handle, Binding::Failure> {
    if (status != Binding::Status::Satisfied) {
      return static_cast<Binding::Failure>(status);
    }

    if constexpr (__is_same(Contract, Counter)) {
      static const counter_operations operations = {
        [](const void*, U64 amount) { return amount; },
      };
      return Counter::Handle(nullptr, operations);
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
  ASSERT(foreign.api.query.bind != nullptr);

  // Known native ownership can supply the same Handle without UUID exchanges.
  // Unsupported delegates; rejection and pending cannot escape through the
  // fallback, even though that foreign provider would satisfy the contract.
  const Binding::Status statuses[] = {
    Binding::Status::Satisfied, Binding::Status::Unsupported,
    Binding::Status::Rejected, Binding::Status::Pending};
  for (auto status : statuses) {
    foreign.api.reset(TTX_BINDING_SATISFIED, TTX_BINDING_SATISFIED, 0, 0);
    const Native native(Query(foreign.api.query), status);
    Simulacra::fulfill<Counter>(native).visit(
        [&](const Counter::Handle& counter) {
          EXPECT(status == Binding::Status::Satisfied ||
                 status == Binding::Status::Unsupported);
          EXPECT_EQ(counter.add(9), U64(9));
        },
        [&](Binding::Failure failure) {
          EXPECT(status == Binding::Status::Rejected ||
                 status == Binding::Status::Pending);
          EXPECT(static_cast<U8>(failure) == static_cast<U8>(status));
        });
    EXPECT_EQ(
        foreign.api.statistics().queries,
        status == Binding::Status::Unsupported ? U64(1) : U64(0));
  }
}
