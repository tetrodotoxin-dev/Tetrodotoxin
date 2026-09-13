// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

#include "perimortem/utility/result.hpp"

#include "ttx/semantic/binding.h"

namespace Ttx::Semantic {

// The dispatch boundary receives a runtime contract identity, so its return
// type cannot name the selected operation table at compile time. Binding
// carries the successful state and table through that boundary. The caller
// then recovers the typed query view for the contract it requested.
//
// Negotiation returns a Result containing either this pair or a Failure. An
// unsupported question may pass to the next policy, while pending and rejected
// answers stop there. Those outcomes belong to the Result rather than to a
// partially populated Binding.
class Binding {
 public:
  enum class Status : U8 {
    Satisfied = TTX_BINDING_SATISFIED,
    Unsupported = TTX_BINDING_UNSUPPORTED,
    Pending = TTX_BINDING_PENDING,
    Rejected = TTX_BINDING_REJECTED,
  };

  // The C status must represent success alongside refusal. A C++ Result
  // already carries a successful Binding, so its error alternative contains
  // only the outcomes that prevented that binding from being established.
  enum class Failure : U8 {
    Unsupported = TTX_BINDING_UNSUPPORTED,
    Pending = TTX_BINDING_PENDING,
    Rejected = TTX_BINDING_REJECTED,
  };

  // The ingress checks the status before accepting this successful pair.
  // A useful optimization is keeping its C value directly to avoid a second
  // representation of the state and table when a native provider returns
  // through that same ingress.
  explicit constexpr Binding(ttx_binding value) : value(value) {}

  constexpr auto get_abi() const -> ttx_binding { return value; }

  // All foreign negotiation entries admit the same status and pair so keeping
  // this check gated at ingress with a Result wrap prevents a failed or partial
  // response from becoming a typed view, without adding additional validation
  // to each operation call. Otherwise it's easy for untyped C values to become
  // SIGSEVs if the caller forgets the whole handshake.
  static auto accept(ttx_binding_status status, ttx_binding value)
      -> Perimortem::Utility::Result<Binding, Failure>;

  template <typename Contract>
  static constexpr auto provide(
      const void* source,
      const typename Contract::Operations& operations) -> Binding {
    return Binding({source, &operations});
  }

  // The dispatch owner must have matched Contract's identity before supplying
  // the table. Supplying a different table is undefined behavior, just as an
  // incompatible function pointer in an operation table would be.
  template <typename Contract>
  constexpr auto get() const -> typename Contract::Handle {
    return typename Contract::Handle(
        value.source,
        *static_cast<const typename Contract::Operations*>(value.operations));
  }

 private:
  ttx_binding value;
};

}  // namespace Ttx::Semantic
