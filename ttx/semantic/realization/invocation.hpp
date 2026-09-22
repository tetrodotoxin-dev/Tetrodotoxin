// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/semantic/realization/invocation.h"
#include "ttx/semantic/negotiation/query.hpp"

namespace Ttx::Semantic::Realization {

// Invocation retains the agreed operation record so repeated calls can use its
// thunk directly without another UUID lookup, signature walk or table transfer.
// The publication supplying its receiver and code outlives every invocation.
class Invocation {
 public:
  auto connect(
      Negotiation::Query instance, Perimortem::System::Uuid contract,
      const Data::Form::Representation& inputs,
      const Data::Form::Representation& outputs) -> Negotiation::Binding::Status;

  auto invoke(const void* inputs, void* outputs) const -> Data::Status {
    return static_cast<Data::Status>(call.invoke(call.receiver, inputs, outputs));
  }

  auto close() -> void { call = {}; }

 private:
  ttx_invocation call = {};
};

}  // namespace Ttx::Semantic::Realization

TTX_DATA_RECORD(
    ttx_invocation,
    TTX_DATA_MEMBER(ttx_invocation, receiver),
    TTX_DATA_MEMBER(ttx_invocation, inputs),
    TTX_DATA_MEMBER(ttx_invocation, outputs),
    TTX_DATA_MEMBER(ttx_invocation, invoke));
