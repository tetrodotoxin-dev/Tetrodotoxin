// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/semantic/thunk.hpp"

using namespace Perimortem;
using namespace Ttx::Semantic;

auto Thunk::Handle::fulfill(
    System::Uuid contract,
    Convention convention,
    const Ttx::Data::Form::Representation& representation) const
    -> Utility::Result<Binding, Binding::Failure> {
  ttx_binding output = {};
  const auto status = operations.fulfill(
      source, contract, static_cast<U32>(convention), &representation, &output);
  return Binding::accept(status, output);
}
