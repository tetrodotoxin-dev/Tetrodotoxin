// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/function.h"
#include "tetrodotoxin/model/execution/layout.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Execution {

// A function publishes semantic parameters, results and its execution body.
// Its operation identity names the behavior later realized by a terminal.
// The body may be Unknown while a supplying policy is still interpreting it.
// Binding this description neither invokes the function nor requires source
// provenance. All returned edges borrow the declaration publication.
class Function {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_EXECUTION_FUNCTION_ID_HIGH,
    TETRODOTOXIN_MODEL_EXECUTION_FUNCTION_ID_LOW,
  };
  using Api = tetrodotoxin_model_execution_function;

  explicit constexpr Function(Api api) : api(api) {}

  auto get_operation() const -> Perimortem::System::Uuid {
    return Perimortem::System::Uuid(api.get_operation(api.source));
  }

  auto get_parameters() const -> Layout {
    return Layout(api.get_parameters(api.source));
  }

  auto get_results() const -> Layout {
    return Layout(api.get_results(api.source));
  }

  auto get_body() const -> Ttx::Concept::Abstract {
    return Ttx::Concept::Abstract(api.get_body(api.source));
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Model::Execution

TTX_DATA_RECORD(
    tetrodotoxin_model_execution_function,
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_function, source),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_function, get_operation),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_function, get_parameters),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_function, get_results),
    TTX_DATA_MEMBER(tetrodotoxin_model_execution_function, get_body));
