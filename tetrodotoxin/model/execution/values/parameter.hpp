// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/field.hpp"
#include "tetrodotoxin/model/execution/parameter.hpp"
#include "ttx/concept/domain.hpp"

namespace Tetrodotoxin::Model::Execution::Values {

// A use keeps the declaration field, including a policy attached by its owner.
// The terminal relates that same field to its admitted input position.
class Parameter {
 public:
  constexpr explicit Parameter(Ttx::Concept::Abstract field) : field(field) {}
  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return id == Execution::Parameter::contract_id ||
                   id == Ttx::Concept::Domain::contract_id
               ? Status::Satisfied
               : Status::Unsupported;
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Contract = Execution::Parameter;
    if (id == Ttx::Concept::Domain::contract_id) {
      static const Ttx::Concept::Domain::Operations operations = {
        [](const void* self, ttx_abstract* result) -> ttx_binding_status {
          return static_cast<const Parameter*>(self)->field.bind<Field>().visit(
              [&](Field field) -> ttx_binding_status {
                *result = field.get_type().get_abi();
                return TTX_BINDING_SATISFIED;
              },
              [](Ttx::Semantic::Negotiation::Binding::Failure failure) {
                return static_cast<ttx_binding_status>(failure);
              });
        }};
      const Ttx::Concept::Domain::Api api(this, &operations);
      return Ttx::Semantic::Negotiation::Binding::provide<Ttx::Concept::Domain>(
          api, output);
    }

    if (id != Contract::contract_id) {
      return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
    }

    const Contract::Api api = {
      this, [](const void* self) -> ttx_abstract {
        return static_cast<const Parameter*>(self)->field.get_abi();
      }};
    return Ttx::Semantic::Negotiation::Binding::provide<Contract>(api, output);
  }

 private:
  Ttx::Concept::Abstract field;
};

}  // namespace Tetrodotoxin::Model::Execution::Values
