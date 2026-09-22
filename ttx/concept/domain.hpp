// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/domain.h"

namespace Ttx::Concept {

// Domain is a question a provider can answer alongside its value operations.
// Memory, constants and computed expressions can supply the same relationship
// while retaining their different representations and policies. Fitting keeps
// the original producer and uses this edge only for the requested relation.
class Domain {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_DOMAIN_ID_HIGH,
    TTX_DOMAIN_ID_LOW,
  };

  using Api = ttx_domain;
  using Operations = ttx_domain_ops;
  using Failure = Semantic::Negotiation::Binding::Failure;
  using Answer = Perimortem::Utility::Result<Abstract, Failure>;

  explicit constexpr Domain(Api api) : api(api) {}

  auto get_domain() const -> Answer {
    ttx_abstract result = {};
    switch (api.operations->get_domain(api.source, &result)) {
    case TTX_BINDING_SATISFIED:
      if (result.source != nullptr && result.operations != nullptr) {
        return Abstract(result);
      }

      return Failure::Rejected;
    case TTX_BINDING_UNSUPPORTED:
      return Failure::Unsupported;
    case TTX_BINDING_PENDING:
      return Failure::Pending;
    default:
      return Failure::Rejected;
    }
  }

  template <typename Provider>
  static auto provide(const Provider& provider, Data::Form::Storage requested)
      -> Semantic::Negotiation::Binding::Status {
    static const Operations operations = {
      [](const void* source, ttx_abstract* result) -> ttx_binding_status {
        return static_cast<const Provider*>(source)->get_domain().visit(
            [&](Abstract domain) -> ttx_binding_status {
              *result = domain.get_abi();
              return TTX_BINDING_SATISFIED;
            },
            [](Failure failure) -> ttx_binding_status {
              return static_cast<ttx_binding_status>(failure);
            });
      },
    };
    return Semantic::Negotiation::Binding::provide<Domain>(
        Api(&provider, &operations), requested);
  }

 private:
  Api api;
};

}  // namespace Ttx::Concept

TTX_DATA_RECORD(ttx_domain_ops, TTX_DATA_MEMBER(ttx_domain_ops, get_domain));
TTX_DATA_RECORD(
    ttx_domain,
    TTX_DATA_MEMBER(ttx_domain, source),
    TTX_DATA_MEMBER(ttx_domain, operations));
