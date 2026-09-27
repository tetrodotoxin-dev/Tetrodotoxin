// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/layout.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Execution {

// An execution Layout orders semantic positions rather than bytes. Each subject
// carries its own value, Type or policy. The caller queries only indices below
// size, and every returned Abstract borrows the publication. Physical packing
// is a separate realization through Type Storage and TTX Data.
// This borrowed observation keeps its size and subjects stable while consumed.
class Layout {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_EXECUTION_LAYOUT_ID_HIGH,
    TETRODOTOXIN_MODEL_EXECUTION_LAYOUT_ID_LOW,
  };
  using Api = tetrodotoxin_model_execution_layout;

  explicit constexpr Layout(Api api) : api(api) {}

  constexpr auto get_abi() const -> Api { return api; }

  auto get_size() const -> Count { return api.get_size(api.source); }

  auto get_subject(Count index) const -> Ttx::Concept::Abstract {
    return Ttx::Concept::Abstract(api.get_subject(api.source, index));
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Model::Execution

TTX_DATA_RECORD(
    tetrodotoxin_model_execution_layout,
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_layout, source),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_layout, get_size),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_layout, get_subject));
