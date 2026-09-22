// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/answers/constant.h"

namespace Ttx::Concept::Answers {

// Constant is used as a policy layer to signal that the resolved semantic
// question is final. Once a concept resolves to a Constant, the implied graph
// edge is axiomatic for the graph lifetime and is contractually promised by the
// provider that consumers may retain it without evaluating the concept again.
//
// Constant does not imply that all outgoing routes are Constant. The policy
// _only_ guarantees the edge that was semantically represented by the consumer
// and provider pair.
class Constant {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_CONSTANT_ID_HIGH,
    TTX_CONSTANT_ID_LOW,
  };
  using Api = void;
};

}  // namespace Ttx::Concept::Answers
