// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/field.hpp"

namespace Tetrodotoxin::Model::Execution::Fields {

// This field retains only its semantic Type edge. A source declaration or an
// export policy can wrap it without becoming part of the field's construction.
class Value {
 public:
  constexpr explicit Value(Ttx::Concept::Abstract type) : type(type) {}
  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return id == Execution::Field::contract_id ? Status::Satisfied
                                               : Status::Unsupported;
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    if (id != Field::contract_id) {
      return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
    }

    const Field::Api api = {
      this, [](const void* self) -> ttx_abstract {
        return static_cast<const Value*>(self)->type.get_abi();
      }};
    return Ttx::Semantic::Negotiation::Binding::provide<Field>(api, output);
  }

 private:
  Ttx::Concept::Abstract type;
};

}  // namespace Tetrodotoxin::Model::Execution::Fields
