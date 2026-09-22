// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/function.hpp"

namespace Tetrodotoxin::Model::Execution::Functions {

// Function retains the execution facts supplied by its builder. A Library
// interpreter, another dialect or a package loader can build the same model.
// They retain their own provenance and publication policies around this body.
class Function {
 public:
  constexpr Function(
      Perimortem::System::Uuid operation,
      Execution::Layout parameters,
      Execution::Layout results,
      Ttx::Concept::Abstract body)
      : operation(operation),
        parameters(parameters),
        results(results),
        body(body) {}
  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return id == Execution::Function::contract_id ? Status::Satisfied
                                                  : Status::Unsupported;
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Contract = Execution::Function;
    if (id != Contract::contract_id) {
      return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
    }

    const Contract::Api api = {
      this,
      [](const void* self) -> perimortem_uuid {
        return static_cast<const Function*>(self)->operation;
      },
      [](const void* self) -> tetrodotoxin_model_execution_layout {
        return static_cast<const Function*>(self)->parameters.get_abi();
      },
      [](const void* self) -> tetrodotoxin_model_execution_layout {
        return static_cast<const Function*>(self)->results.get_abi();
      },
      [](const void* self) -> ttx_abstract {
        return static_cast<const Function*>(self)->body.get_abi();
      }};
    return Ttx::Semantic::Negotiation::Binding::provide<Contract>(api, output);
  }

 private:
  Perimortem::System::Uuid operation;
  Execution::Layout parameters;
  Execution::Layout results;
  Ttx::Concept::Abstract body;
};

}  // namespace Tetrodotoxin::Model::Execution::Functions
