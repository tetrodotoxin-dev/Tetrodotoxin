// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/model/execution/assignments/memory.hpp"

#include "ttx/semantic/flows/copy.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Tetrodotoxin::Model;

static auto transfer(Abstract value, Ttx::Data::Form::Storage target)
    -> Utility::Result<Ttx::Data::Status, Binding::Failure> {
  using Result = Utility::Result<Ttx::Data::Status, Binding::Failure>;
  return value.bind<Execution::Value>().visit(
      [&](Execution::Value value) -> Result {
        Ttx::Semantic::Transport::Flow flow;
        const auto status = flow.connect(
            decltype(flow)::reader(target.get_representation()),
            value.get_value());
        switch (status) {
        case decltype(flow)::Status::Success:
          return Ttx::Semantic::Flows::Copy::flow(flow, target);
        case decltype(flow)::Status::BindingPending:
          return Binding::Failure::Pending;
        case decltype(flow)::Status::Unsupported:
          return Binding::Failure::Unsupported;
        case decltype(flow)::Status::Rejected:
        case decltype(flow)::Status::Incompatible:
          return Binding::Failure::Rejected;
        case decltype(flow)::Status::Invalid:
        case decltype(flow)::Status::Bounds:
        case decltype(flow)::Status::Overflow:
        case decltype(flow)::Status::Denied:
        case decltype(flow)::Status::Busy:
        case decltype(flow)::Status::IoError:
          return static_cast<Ttx::Data::Status>(status);
        }

        return Binding::Failure::Rejected;
      },
      [](Binding::Failure failure) -> Result { return failure; });
}

auto Execution::Assignments::Memory::assign(Abstract value) const
    -> Utility::Result<Ttx::Data::Status, Binding::Failure> {
  using Result = Utility::Result<Ttx::Data::Status, Binding::Failure>;
  // Admission belongs to this transaction. No saved Type proof or previous
  // successful assignment can authorize the next value arriving at this edge.
  return type.bind<Type::Policies::Conversion>().visit(
      [&](Type::Policies::Conversion conversion) -> Result {
        return conversion.convert(value).visit(
            [&](Abstract projected) -> Result {
              return transfer(projected, storage);
            },
            [](Binding::Failure failure) -> Result { return failure; });
      },
      [](Binding::Failure failure) -> Result { return failure; });
}

auto Execution::Assignments::Memory::supports(System::Uuid id) const
    -> Binding::Status {
  return id == Execution::Assignment::contract_id ||
                 id == Execution::Value::contract_id ||
                 id == Ttx::Concept::Domain::contract_id ||
                 id == Ttx::Semantic::Transport::Direct::Access::contract_id
             ? Binding::Status::Satisfied
             : Binding::Status::Unsupported;
}

auto Execution::Assignments::Memory::bind_interface(
    System::Uuid id,
    Ttx::Data::Form::Storage output) const -> Binding::Status {
  if (id == Execution::Assignment::contract_id) {
    const Execution::Assignment::Api api = {
      this,
      [](const void* self, ttx_abstract value,
         ttx_data_status* result) -> ttx_binding_status {
        return static_cast<const Memory*>(self)
            ->assign(Abstract(value))
            .visit(
                [&](Ttx::Data::Status status) -> ttx_binding_status {
                  *result = static_cast<ttx_data_status>(status);
                  return TTX_BINDING_SATISFIED;
                },
                [](Binding::Failure failure) {
                  return static_cast<ttx_binding_status>(failure);
                });
      }};
    return Binding::provide<Execution::Assignment>(api, output);
  }

  if (id == Ttx::Concept::Domain::contract_id) {
    static const Ttx::Concept::Domain::Operations operations = {
      [](const void* self, ttx_abstract* result) -> ttx_binding_status {
        *result = static_cast<const Memory*>(self)->type.get_abi();
        return TTX_BINDING_SATISFIED;
      }};
    const Ttx::Concept::Domain::Api api(this, &operations);
    return Binding::provide<Ttx::Concept::Domain>(api, output);
  }

  if (id == Execution::Value::contract_id) {
    const Execution::Value::Api api = {
      this, [](const void* self) -> ttx_semantic_query {
        return Abstract::provide(*static_cast<const Memory*>(self)).get_query();
      }};
    return Binding::provide<Execution::Value>(api, output);
  }

  if (id == Ttx::Semantic::Transport::Direct::Access::contract_id) {
    using Ttx::Semantic::Transport::Direct;
    static const Direct::Access::Operations operations = {
      [](const void* self) -> const ttx_representation* {
        return &static_cast<const Memory*>(self)->storage.get_representation();
      },
      [](const void* self) -> const void* {
        return static_cast<const Memory*>(self)->storage.get_bytes().get_data();
      }};
    return Binding::provide<Direct::Access>(
        Direct::Access::Api(this, &operations), output);
  }

  return Binding::Status::Unsupported;
}
