// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/answers/unknown.h"

namespace Ttx::Concept::Answers {

// Unknown is the provisional answer to a semantic question that the current
// graph cannot settle. Since repeating the original question may later produce
// a factual identity under a different observational state, Unknown can't be
// used to assume the resolution would never produce a valid answer.
//
// For explicit rejection None should be used instead.
class Unknown {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_UNKNOWN_ID_HIGH,
    TTX_UNKNOWN_ID_LOW,
  };
  using Api = void;
  static auto get_unknown() -> Abstract;
  auto get_data() const -> Perimortem::Core::View::Bytes;
  auto supports(Perimortem::System::Uuid id) const
      -> Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Data::Form::Storage requested) const
      -> Semantic::Negotiation::Binding::Status;
  auto resolve_concept(Perimortem::Core::View::Bytes route) const -> Abstract;
};

}  // namespace Ttx::Concept::Answers
