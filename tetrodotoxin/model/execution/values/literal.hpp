// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/execution/constant.hpp"
#include "tetrodotoxin/model/execution/value.hpp"
#include "ttx/concept/domain.hpp"
#include "ttx/semantic/transport/direct.hpp"

namespace Tetrodotoxin::Model::Execution::Values {

// A literal owns its native payload and borrows the policy that supplies its
// Type. Domain exposes that Type and Value supplies the data Query. Constant
// promises the payload is immutable, so another provider can expose the same
// facts through Block without keeping the same native storage.
// The caller keeps this owner alive while any acquired Direct view is in use.
template <typename Payload>
class Literal {
 public:
  constexpr Literal(Ttx::Concept::Abstract type, const Payload& value)
      : type(type), value(value) {}
  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return id == Constant::contract_id ||
                   id == Ttx::Concept::Domain::contract_id ||
                   id == Execution::Value::contract_id ||
                   id == Ttx::Semantic::Transport::Direct::Access::contract_id
               ? Status::Satisfied
               : Status::Unsupported;
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using namespace Ttx::Semantic::Negotiation;
    using Ttx::Semantic::Transport::Direct;
    if (id == Constant::contract_id) {
      return Binding::marker(output);
    }

    if (id == Ttx::Concept::Domain::contract_id) {
      static const Ttx::Concept::Domain::Operations operations = {
        [](const void* self, ttx_abstract* result) -> ttx_binding_status {
          *result = static_cast<const Literal*>(self)->type.get_abi();
          return TTX_BINDING_SATISFIED;
        }};
      const Ttx::Concept::Domain::Api api(this, &operations);
      return Binding::provide<Ttx::Concept::Domain>(api, output);
    }

    if (id == Execution::Value::contract_id) {
      const Execution::Value::Api api = {
        this, [](const void* self) -> ttx_semantic_query {
          return Ttx::Concept::Abstract::provide(
                     *static_cast<const Literal*>(self))
              .get_query();
        }};
      return Binding::provide<Execution::Value>(api, output);
    }

    if (id == Direct::Access::contract_id) {
      static const Direct::Access::Operations operations = {
        [](const void*) -> const ttx_representation* {
          return &Ttx::Data::Form::Compiled<Ttx::Data::Form::Native<
              Payload>::reference>::get_representation();
        },
        [](const void* self) -> const void* {
          return &static_cast<const Literal*>(self)->value;
        }};
      return Binding::provide<Direct::Access>(
          Direct::Access::Api(this, &operations), output);
    }

    return Binding::Status::Unsupported;
  }

 private:
  Ttx::Concept::Abstract type;
  Payload value;
};

}  // namespace Tetrodotoxin::Model::Execution::Values
