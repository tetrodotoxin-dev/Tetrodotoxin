// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/answers/none.h"

namespace Ttx::Concept::Answers {

// None is the axiomatic answer that a completed concept has no semantic value.
// Its named routes terminate on itself, while its marker binding lets another
// provider recognize completed absence without borrowing any operations.
class None {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_NONE_ID_HIGH,
    TTX_NONE_ID_LOW,
  };
  using Api = void;
  static auto get_none() -> Abstract;
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
