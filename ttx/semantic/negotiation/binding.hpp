// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/form/compiled.hpp"
#include "ttx/data/form/native.hpp"
#include "ttx/data/form/storage.hpp"
#include "ttx/semantic/negotiation/binding.h"

namespace Ttx::Semantic::Negotiation::Binding {

// Status describes the exchange, independently of the shape of its result.
// Typed C++ consumers keep their actual API record and return a Result carrying
// only a Failure when negotiation could not establish that record.
enum class Status : U8 {
  Satisfied = TTX_BINDING_SATISFIED,
  Unsupported = TTX_BINDING_UNSUPPORTED,
  Pending = TTX_BINDING_PENDING,
  Rejected = TTX_BINDING_REJECTED,
};

enum class Failure : U8 {
  Unsupported = TTX_BINDING_UNSUPPORTED,
  Pending = TTX_BINDING_PENDING,
  Rejected = TTX_BINDING_REJECTED,
};

template <typename Contract>
auto representation() -> const Data::Form::Representation& {
  if constexpr (__is_same(typename Contract::Api, void)) {
    static constexpr auto empty = Data::Form::Schema::composite(
        Perimortem::Core::View::Vector<Data::Form::Schema::Position>(), 0);
    return Data::Form::Compiled<empty>::get_representation();
  } else {
    return Data::Form::Compiled<
        Data::Form::Native<typename Contract::Api>::reference>::get_representation();
  }
}

template <typename Api>
auto provide(
    const Api& api, const Data::Form::Representation& actual,
    Data::Form::Storage requested) -> Status {
  return static_cast<Status>(
      ttx_binding_provide(&actual, &api, requested.get_abi()));
}

template <typename Contract>
auto provide(const typename Contract::Api& api, Data::Form::Storage requested)
    -> Status {
  return provide(api, representation<Contract>(), requested);
}

inline auto marker(Data::Form::Storage requested) -> Status {
  return static_cast<Status>(ttx_binding_marker(requested.get_abi()));
}

}  // namespace Ttx::Semantic::Negotiation::Binding
