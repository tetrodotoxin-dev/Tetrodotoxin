// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/semantic/realization/invocation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/ttx/semantic/measurement.hpp"

#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

#include "ttx/data/form/compiled.hpp"
#include "ttx/semantic/flows/copy.hpp"
#include "ttx/semantic/transport/flow.hpp"
#include "validation/unit_tests/ttx/semantic/fixtures/invocation_provider.h"

using namespace Perimortem;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Realization;
using namespace Ttx::Semantic::Transport;
using Ttx::Data::Form::Schema;

static Validation::Harness InvocationTests = {.name = "TTX::Invocation"_view};
static constexpr auto integer = Schema::primitive(Schema::Value::S64);
static constexpr auto real = Schema::primitive(Schema::Value::R64);
static constexpr auto flag = Schema::primitive(Schema::Value::U8);
static constexpr Schema::Position positions[] = {
  Schema::Position(integer, __builtin_offsetof(invocation_input, value)),
  Schema::Position(real, __builtin_offsetof(invocation_input, scale)),
  Schema::Position(flag, __builtin_offsetof(invocation_input, negate)),
};
static constexpr auto inputs = Schema::composite(
    positions,
    sizeof(invocation_input),
    alignof(invocation_input));
static constexpr System::Uuid operation{
  INVOCATION_METHOD_HIGH, INVOCATION_METHOD_LOW};

class ForeignInvocation {
 public:
  ForeignInvocation() {
    char path[4096];
    const auto size = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (size <= 0) {
      return;
    }

    path[size] = 0;
    auto* name = strrchr(path, '/');
    if (!name || sizeof(path) - (name + 1 - path) <
                     sizeof("libinvocation_provider.so")) {
      return;
    }

    memcpy(
        name + 1, "libinvocation_provider.so",
        sizeof("libinvocation_provider.so"));
    module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (module) {
      const auto open = reinterpret_cast<decltype(&invocation_provider_open)>(
          dlsym(module, "invocation_provider_open"));
      if (open) {
        api = open(
            ttx_invocation_representation(), &input_form(), &output_form());
      }
    }
  }

  ~ForeignInvocation() {
    if (module) {
      dlclose(module);
    }
  }

  static auto input_form() -> const Ttx::Data::Form::Representation& {
    return Ttx::Data::Form::Compiled<inputs>::get_representation();
  }
  static auto output_form() -> const Ttx::Data::Form::Representation& {
    return Ttx::Data::Form::Compiled<real>::get_representation();
  }
  auto query() -> Query {
    return Query(
        {this,
         [](const void* source, perimortem_uuid id,
            ttx_storage requested) -> ttx_binding_status {
           const auto& self = *static_cast<const ForeignInvocation*>(source);
           if (System::Uuid(id) != operation) {
             return TTX_BINDING_UNSUPPORTED;
           }

           // This provider policy composes ordinary Flow and Copy. The C module
           // chooses its transport; binding contains no callable-specific path.
           // The copied receiver belongs to the module, so releasing a Shared
           // loan of the API record does not invalidate that receiver.
           Flow flow;
           const auto status = flow.connect(
               Flow::reader(*requested.representation), Query(self.api.query));
           if (status == Flow::Status::Unsupported) {
             return TTX_BINDING_UNSUPPORTED;
           }
           if (status == Flow::Status::BindingPending) {
             return TTX_BINDING_PENDING;
           }
           if (status != Flow::Status::Success) {
             return TTX_BINDING_REJECTED;
           }
           const auto copied = Ttx::Semantic::Flows::Copy::flow(
               flow, Ttx::Data::Form::Storage(requested));
           return copied == Ttx::Data::Status::Success ? TTX_BINDING_SATISFIED
                                                       : TTX_BINDING_REJECTED;
         },
         [](const void*, perimortem_uuid id) -> ttx_binding_status {
           return System::Uuid(id) == operation ? TTX_BINDING_SATISFIED
                                                : TTX_BINDING_UNSUPPORTED;
         }});
  }

  auto connect(Invocation& call) -> Binding::Status {
    return call.connect(query(), operation, input_form(), output_form());
  }

  invocation_fixture api = {};

 private:
  void* module = nullptr;
};

PERIMORTEM_UNIT_TEST(InvocationTests, foreign_transports) {
  ForeignInvocation foreign;
  ASSERT(foreign.api.query.bind);
  for (U8 protocol = 0; protocol != 4; ++protocol) {
    foreign.api.configure(protocol, TTX_BINDING_SATISFIED, 0);
    Invocation call;
    ASSERT(foreign.connect(call) == Binding::Status::Satisfied);
    const auto established = foreign.api.statistics();

    // Preparation and Data transfer are deliberately outside measurement.
    // A private C receiver adds two before scaling. The independent arithmetic
    // answer catches lost fields, wrong offsets and incorrect receiver use.
    const invocation_input arguments{5, 1.5, 1};
    R64 answer = 0;
    Validation::FlowTests::Measurement measurement;
    for (Count i = 0; i != 1000000; ++i) {
      ASSERT(call.invoke(&arguments, &answer) == Ttx::Data::Status::Success);
    }
    measurement.stop();
    EXPECT_EQ(answer, R64(-10.5));
    EXPECT_EQ(measurement.get_allocations(), Count(0));
    EXPECT_EQ(measurement.get_copies(), Count(0));

    const auto observed = foreign.api.statistics();
    EXPECT_EQ(observed.binds, established.binds);
    EXPECT_EQ(observed.transfers, established.transfers);
    EXPECT_EQ(observed.calls, U64(1000000));
    EXPECT_EQ(observed.releases, U64(protocol == 1));
    call.close();
    EXPECT_EQ(foreign.api.statistics().releases, U64(protocol == 1));
  }
}

PERIMORTEM_UNIT_TEST(InvocationTests, refusal_and_retry) {
  ForeignInvocation foreign;
  ASSERT(foreign.api.query.bind);
  Invocation call;

  // A payload mismatch is rejected before invoking the operation. Sharing
  // the same extent does not make S64 and R64 interchangeable payloads.
  EXPECT(
      call.connect(
          foreign.query(), operation, ForeignInvocation::input_form(),
          Ttx::Data::Form::Compiled<integer>::get_representation()) ==
      Binding::Status::Rejected);
  EXPECT_EQ(foreign.api.statistics().calls, U64(0));
  const ttx_binding_status failures[] = {
    TTX_BINDING_UNSUPPORTED, TTX_BINDING_PENDING, TTX_BINDING_REJECTED};
  const Binding::Status expected[] = {
    Binding::Status::Unsupported, Binding::Status::Pending,
    Binding::Status::Rejected};
  for (U32 i = 0; i != 3; ++i) {
    foreign.api.configure(0, failures[i], 0);
    EXPECT(foreign.connect(call) == expected[i]);
    // Unsupported allows the provider's ordinary Flow to try every protocol;
    // a pending or rejected policy stops after the first request.
    EXPECT_EQ(
        foreign.api.statistics().binds,
        U64(failures[i] == TTX_BINDING_UNSUPPORTED ? 4 : 1));
  }

  // A failed materialization never publishes a callable record. A corrected
  // retry and a successful replacement both use the same binding operation.
  for (U8 protocol = 2; protocol != 4; ++protocol) {
    foreign.api.configure(protocol, TTX_BINDING_SATISFIED, 1);
    EXPECT(foreign.connect(call) == Binding::Status::Rejected);
    foreign.api.configure(protocol, TTX_BINDING_SATISFIED, 0);
    EXPECT(foreign.connect(call) == Binding::Status::Satisfied);
    EXPECT(foreign.connect(call) == Binding::Status::Satisfied);
    call.close();
  }
}
