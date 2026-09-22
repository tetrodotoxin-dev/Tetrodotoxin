// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/assignment.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Execution {

// Write authority belongs to the encountered destination. A retained binding
// can remain callable while a private or stage policy changes its answer, so
// every assign call asks about its actual source in the current transaction.
// A destination may convert that value before writing without making conversion
// itself grant write access.
//
// The returned binding status describes admission. Only Satisfied supplies a
// transfer outcome, which reports whether the admitted write completed. This
// keeps Pending or a policy refusal distinct from an I/O failure after a write
// began. A failed transfer supplies no valid result and may have touched the
// destination. Atomic commit or rollback belongs to the destination policy.
class Assignment {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_EXECUTION_ASSIGNMENT_ID_HIGH,
    TETRODOTOXIN_MODEL_EXECUTION_ASSIGNMENT_ID_LOW,
  };
  using Api = tetrodotoxin_model_execution_assignment;
  explicit constexpr Assignment(Api api) : api(api) {}
  constexpr auto get_abi() const -> Api { return api; }

  auto assign(Ttx::Concept::Abstract value) const -> Perimortem::Utility::
      Result<Ttx::Data::Status, Ttx::Semantic::Negotiation::Binding::Failure> {
    ttx_data_status outcome = TTX_DATA_INVALID;
    const auto status = api.assign(api.source, value.get_abi(), &outcome);
    if (status != TTX_BINDING_SATISFIED) {
      return static_cast<Ttx::Semantic::Negotiation::Binding::Failure>(status);
    }

    return static_cast<Ttx::Data::Status>(outcome);
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Model::Execution

TTX_DATA_RECORD(
    tetrodotoxin_model_execution_assignment,
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_assignment, source),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_assignment, assign));
