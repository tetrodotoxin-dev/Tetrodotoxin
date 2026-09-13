// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/semantic/binding.hpp"

using namespace Perimortem;
using namespace Ttx::Semantic;

auto Binding::accept(ttx_binding_status status, ttx_binding value)
    -> Utility::Result<Binding, Failure> {
  switch (static_cast<Status>(status)) {
  case Status::Satisfied:
    if (value.operations) {
      return Binding(value);
    }

    return Failure::Rejected;
  case Status::Unsupported:
    return Failure::Unsupported;
  case Status::Pending:
    return Failure::Pending;
  default:
    return Failure::Rejected;
  }
}
