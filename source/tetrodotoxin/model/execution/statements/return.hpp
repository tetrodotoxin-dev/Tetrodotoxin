// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/return.hpp"

namespace Tetrodotoxin::Model::Execution::Statements {

// Return borrows the selected value flow. Its meaning is independent of the
// syntax that selected those values and of the terminal realizing the body.
class Return {
 public:
  constexpr explicit Return(Execution::Layout values) : values(values) {}
  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return id == Execution::Return::contract_id ? Status::Satisfied
                                                : Status::Unsupported;
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Contract = Execution::Return;
    if (id != Contract::contract_id) {
      return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
    }

    const Contract::Api api = {
      this, [](const void* self) -> tetrodotoxin_model_execution_layout {
        return static_cast<const Return*>(self)->values.get_abi();
      }};
    return Ttx::Semantic::Negotiation::Binding::provide<Contract>(api, output);
  }

 private:
  Execution::Layout values;
};

}  // namespace Tetrodotoxin::Model::Execution::Statements
