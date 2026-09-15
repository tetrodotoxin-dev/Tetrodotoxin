// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/semantic/negotiation/query.hpp"

namespace Ttx::Semantic::Realization {

// A simulacrum supplies the answers promised by a contract. Its concrete API
// is acquired through the same checked storage binding as any other record.
// C++ can retain the resulting typed view and invoke it without repeating UUID
// negotiation or inspecting the provider's private object representation.
class Simulacra {
 public:
  template <typename Contract>
  static auto fulfill(Negotiation::Query query) -> Perimortem::Utility::
      Result<Contract, Negotiation::Binding::Failure> {
    return query.template bind<Contract>();
  }

  // An explicitly supplied native owner may already know the exact contract's
  // realization and supply it through its own policy. This shortcut requires
  // that actual Owner, which cannot be recovered by casting a foreign Binding's
  // receiver. Both paths return the same contract, allowing stateless receivers
  // to remain null without using that value as a mode tag. Unsupported requests
  // use the Core path. Rejected and Pending stop here to preserve the owner's
  // policy even when its underlying Query could answer.
  template <typename Contract, typename Owner>
    requires requires(const Owner& owner) {
      owner.template fulfill_native<Contract>();
      owner.get_query();
    }
  static auto fulfill(const Owner& owner) -> Perimortem::Utility::
      Result<Contract, Negotiation::Binding::Failure> {
    using Result = Perimortem::Utility::Result<
        Contract, Negotiation::Binding::Failure>;
    return owner.template fulfill_native<Contract>().visit(
        [](const Contract& contract) -> Result {
          return contract;
        },
        [&](Negotiation::Binding::Failure failure) -> Result {
          if (failure == Negotiation::Binding::Failure::Unsupported) {
            return fulfill<Contract>(owner.get_query());
          }

          return failure;
        });
  }
};

}  // namespace Ttx::Semantic::Realization
