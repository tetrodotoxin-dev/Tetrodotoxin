// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "perimortem/utility/result.hpp"

#include "ttx/semantic/negotiation/binding.hpp"
#include "ttx/semantic/negotiation/query.h"

namespace Ttx::Semantic::Negotiation {

// An enclosing protocol supplies the first semantic contract: the contract to
// query and negotiate semantic contracts.
//
// Since negotiating semantic contracts _requires_ negotation of semantic
// contracts there is a level of bootstrapping that is required to get the first
// source interface. A loaded module can return it from an agreed entry point,
// while a native owner can lend its bind thunk directly. That initial agreement
// makes UUID and representation negotiation possible without requiring either
// participant to construct an Abstract graph. The bind thunk is directly
// accessible under that prearranged lifetime, which gives later contracts a
// starting point for negotiating their own Data access.
//
// Once acquired, Query asks for one contract and returns its bound operations
// in the negotiated storage format. The supplying state and code remain
// borrowed through negotiation and every use of the returned view.
class Query {
 public:
  constexpr Query() = default;

  constexpr explicit Query(ttx_semantic_query value) : value(value) {}

  constexpr operator ttx_semantic_query() const { return value; }

  constexpr auto is_set() const -> Bool { return value.bind != nullptr; }

  auto bind(Perimortem::System::Uuid contract, Data::Form::Storage requested)
      const -> Binding::Status;

  template <typename Contract>
  auto bind() const -> Perimortem::Utility::Result<Contract, Binding::Failure> {
    const auto& form = Binding::representation<Contract>();
    if constexpr (__is_same(typename Contract::Api, void)) {
      const auto status = bind(
          Contract::contract_id,
          Data::Form::Storage(ttx_storage{&form, nullptr, 0}));
      if (status != Binding::Status::Satisfied) {
        return static_cast<Binding::Failure>(status);
      }

      return Contract();
    } else {
      // The actual API type supplies storage, including its alignment. A failed
      // exchange never publishes this local value as a callable interface.
      typename Contract::Api api = {};
      const ttx_storage target{&form, reinterpret_cast<U8*>(&api), sizeof(api)};
      if (ttx_storage_check(target) != TTX_DATA_SUCCESS) {
        return Binding::Failure::Rejected;
      }

      const auto status =
          bind(Contract::contract_id, Data::Form::Storage(target));
      if (status != Binding::Status::Satisfied) {
        return static_cast<Binding::Failure>(status);
      }

      if constexpr (requires { Contract::accept(api); }) {
        if (!Contract::accept(api)) {
          return Binding::Failure::Rejected;
        }
      }

      return Contract(api);
    }
  }

 private:
  ttx_semantic_query value = {};
};

}  // namespace Ttx::Semantic::Negotiation

TTX_DATA_RECORD(
    perimortem_uuid,
    TTX_DATA_MEMBER(perimortem_uuid, high),
    TTX_DATA_MEMBER(perimortem_uuid, low));
TTX_DATA_RECORD(
    ttx_semantic_query,
    TTX_DATA_MEMBER(ttx_semantic_query, source),
    TTX_DATA_MEMBER(ttx_semantic_query, bind));
