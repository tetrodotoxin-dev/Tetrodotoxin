// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stdio.h>

#include "ttx/semantic/negotiation/query.hpp"
#include "validation/unit_tests/ttx/semantic/fixtures/portable_provider.h"
TTX_DATA_RECORD(
    portable_counter,
    TTX_DATA_MEMBER(portable_counter, receiver),
    TTX_DATA_MEMBER(portable_counter, add));
struct Counter {
  using Api = portable_counter;
  static constexpr Perimortem::System::Uuid contract_id =
      Perimortem::System::Uuid(17, 23);
  Api api;
  explicit Counter(Api api) : api(api) {}
};
struct WrongApi {
  const void* receiver;
  U64 (*add)(const void*, U64);
};
TTX_DATA_RECORD(
    WrongApi,
    TTX_DATA_MEMBER(WrongApi, receiver),
    TTX_DATA_MEMBER(WrongApi, add));
struct Wrong {
  using Api = WrongApi;
  static constexpr Perimortem::System::Uuid contract_id =
      Perimortem::System::Uuid(17, 23);
  explicit Wrong(Api) {}
};
// One UUID can describe the intended operation even when its C declaration
// drifted. Refuse the U64 declaration before calling anything, then negotiate
// the actual U32 API and invoke its opaque receiver through the returned thunk.
int main() {
  using namespace Ttx::Semantic::Negotiation;
  const Query query(portable_counter_open());
  if (query.supports<Counter>() != Binding::Status::Satisfied) {
    return 1;
  }
  bool rejected = query.bind<Wrong>().visit(
      [](Wrong) { return false; },
      [](Binding::Failure failure) {
        return failure == Binding::Failure::Rejected;
      });
  if (!rejected || portable_counter_calls()) {
    return 2;
  }
  bool called = query.bind<Counter>().visit(
      [](Counter counter) {
        return counter.api.add(counter.api.receiver, 2) == 42;
      },
      [](Binding::Failure) { return false; });
  if (!called || portable_counter_calls() != 1) {
    return 3;
  }
  puts(
      "PASS: C provider, representation mismatch rejection and typed C++ call");
}
