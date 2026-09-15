// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/semantic/realization/invocation.hpp"

using namespace Ttx::Semantic::Realization;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Data::Form;

PERIMORTEM_C const ttx_representation* ttx_invocation_representation() {
  return &Compiled<Native<ttx_invocation>::reference>::get_representation();
}

auto Invocation::connect(
    Query instance, Perimortem::System::Uuid contract,
    const Representation& inputs, const Representation& outputs) -> Binding::Status {
  ttx_invocation next = {};
  const Storage target(ttx_storage{
      ttx_invocation_representation(), reinterpret_cast<U8*>(&next), sizeof(next)});
  const auto status = instance.bind(contract, target);
  if (status != Binding::Status::Satisfied) {
    return status;
  }

  if (!next.invoke || !next.inputs || !next.outputs ||
      !inputs.compatible(*next.inputs) || !outputs.compatible(*next.outputs)) {
    return Binding::Status::Rejected;
  }

  call = next;
  return Binding::Status::Satisfied;
}
