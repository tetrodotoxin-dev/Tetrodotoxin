// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/layout.hpp"
#include "tetrodotoxin/model/execution/return.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Execution {

// Return transfers its ordered values to the enclosing function's results.
// An empty Layout returns no values. The terminal checks the corresponding
// Types before emitting code. No source statement or lexical context is needed.
class Return {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_EXECUTION_RETURN_ID_HIGH,
    TETRODOTOXIN_MODEL_EXECUTION_RETURN_ID_LOW,
  };
  using Api = tetrodotoxin_model_execution_return;

  explicit constexpr Return(Api api) : api(api) {}

  auto get_values() const -> Layout {
    return Layout(api.get_values(api.source));
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Model::Execution

TTX_DATA_RECORD(
    tetrodotoxin_model_execution_return,
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_return, source),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_return, get_values));
