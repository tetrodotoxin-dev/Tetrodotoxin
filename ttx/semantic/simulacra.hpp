// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/semantic/query.hpp"
#include "ttx/semantic/thunk.hpp"

namespace Ttx::Semantic {

// A simulacrum preserves the questions and answers promised by a contract.
// This consumer acquires that promise as a callable C++ view. Contract supplies
// its UUID, convention, table representation and Handle; the provider chooses
// the implementation and receiver that satisfy them. Neither needs the other's
// native type identity or internal object layout.
//
// Fulfillment performs two exchanges: obtain Thunk through the supplied Query,
// then request the contract's concrete realization. Retain the returned Handle
// with the publication owner to amortize those exchanges over ordinary calls.
// Failure remains distinct from an absent capability so a rejected policy
// cannot be bypassed by looking for a different representation underneath it.
class Simulacra {
 public:
  static auto fulfill(
      Query query,
      Perimortem::System::Uuid contract,
      Thunk::Convention convention,
      const Data::Form::Representation& representation)
      -> Perimortem::Utility::Result<Binding, Binding::Failure> {
    using Result = Perimortem::Utility::Result<Binding, Binding::Failure>;
    return query.bind<Thunk>().visit(
        [&](const Thunk::Handle& protocol) -> Result {
          return protocol.fulfill(contract, convention, representation);
        },
        [](Binding::Failure failure) -> Result { return failure; });
  }

  template <typename Contract>
  static auto fulfill(Query query) -> Perimortem::Utility::
      Result<typename Contract::Handle, Binding::Failure> {
    using Result = Perimortem::Utility::Result<
        typename Contract::Handle, Binding::Failure>;

    return fulfill(
               query, Contract::contract_id, Contract::convention,
               Contract::get_representation())
        .visit(
            [](const Binding& binding) -> Result {
              return binding.get<Contract>();
            },
            [](Binding::Failure failure) -> Result { return failure; });
  }

  // An explicitly supplied native owner may already know the exact contract's
  // realization. Its own policy supplies that view; no receiver obtained from
  // a foreign Binding is cast to recover an Owner. Both paths return the same
  // Handle, so callers need no mode tag and stateless receivers may remain
  // null. Unsupported requests use the Core path. Rejection and pending stop
  // here, preserving the owner's policy even when its underlying Query could
  // answer.
  template <typename Contract, typename Owner>
    requires requires(const Owner& owner) {
      owner.template fulfill_native<Contract>();
      owner.get_query();
    }
  static auto fulfill(const Owner& owner) -> Perimortem::Utility::
      Result<typename Contract::Handle, Binding::Failure> {
    using Result = Perimortem::Utility::Result<
        typename Contract::Handle, Binding::Failure>;
    return owner.template fulfill_native<Contract>().visit(
        [](const typename Contract::Handle& handle) -> Result {
          return handle;
        },
        [&](Binding::Failure failure) -> Result {
          if (failure == Binding::Failure::Unsupported) {
            return fulfill<Contract>(owner.get_query());
          }

          return failure;
        });
  }
};

}  // namespace Ttx::Semantic
