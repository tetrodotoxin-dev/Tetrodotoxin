// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/value.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Execution {

// A value observation can be backed by a buffer, device, generator or remote
// service. Value supplies the data Query for that observation without exposing
// its storage mechanism. Domain separately supplies its current Type. The
// returned Query borrows the publication, and repeated data observations may
// differ unless the producer supplies the Execution Constant promise.
class Value {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_EXECUTION_VALUE_ID_HIGH,
    TETRODOTOXIN_MODEL_EXECUTION_VALUE_ID_LOW,
  };
  using Api = tetrodotoxin_model_execution_value;
  explicit constexpr Value(Api api) : api(api) {}
  constexpr auto get_abi() const -> Api { return api; }

  auto get_value() const -> Ttx::Semantic::Negotiation::Query {
    return Ttx::Semantic::Negotiation::Query(api.get_value(api.source));
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Model::Execution

TTX_DATA_RECORD(
    tetrodotoxin_model_execution_value,
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_value, source),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_value, get_value));
