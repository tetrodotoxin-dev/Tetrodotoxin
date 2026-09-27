// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/policies/conversion.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Type::Policies {

// A receiving Type owns which value observations it can accept. Conversion
// asks that policy for the actual source subject in this transaction. Success
// returns the admitted producer, which may be the source or a provider owned
// projection. No write is implied, and the result grants no permission for a
// later transaction. Retaining an answer without repeating this question needs
// a Constant proof for the particular edge, not merely a retained interface.
//
// Inputs and returned subjects borrow their publications. A provider supplying
// a new projection keeps it alive under that publication's lifetime.
// Unsupported, Pending and Rejected leave output unavailable and preserve the
// current policy.
class Conversion {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_TYPE_CONVERSION_ID_HIGH,
    TETRODOTOXIN_MODEL_TYPE_CONVERSION_ID_LOW,
  };
  using Api = tetrodotoxin_model_type_conversion;
  explicit constexpr Conversion(Api api) : api(api) {}
  constexpr auto get_abi() const -> Api { return api; }

  auto convert(Ttx::Concept::Abstract value) const
      -> Perimortem::Utility::Result<
          Ttx::Concept::Abstract,
          Ttx::Semantic::Negotiation::Binding::Failure> {
    ttx_abstract output = {};
    const auto status = api.convert(api.source, value.get_abi(), &output);
    if (status != TTX_BINDING_SATISFIED) {
      return static_cast<Ttx::Semantic::Negotiation::Binding::Failure>(status);
    }

    return Ttx::Concept::Abstract(output);
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Model::Type::Policies

TTX_DATA_RECORD(
    tetrodotoxin_model_type_conversion,
    TTX_DATA_MEMBER(tetrodotoxin_model_type_conversion, source),
    TTX_DATA_MEMBER(tetrodotoxin_model_type_conversion, convert));
