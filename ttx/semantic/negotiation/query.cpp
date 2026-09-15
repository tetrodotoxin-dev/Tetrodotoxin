// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/semantic/negotiation/query.hpp"

using namespace Perimortem;
using namespace Ttx::Semantic::Negotiation;

auto Query::bind(System::Uuid contract, Ttx::Data::Form::Storage requested) const
    -> Binding::Status {
  if (!value.bind) {
    return Binding::Status::Rejected;
  }

  const auto status = value.bind(value.source, contract, requested.get_abi());
  return status <= TTX_BINDING_REJECTED ? static_cast<Binding::Status>(status)
                                       : Binding::Status::Rejected;
}
