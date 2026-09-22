// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/answers/unknown.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/answers/constant.hpp"

using namespace Ttx;
using namespace Perimortem;

using namespace Ttx::Concept;
using namespace Ttx::Concept::Answers;

auto Unknown::get_data() const -> Core::View::Bytes {
  return "Unknown"_view;
}

auto Unknown::supports(System::Uuid id) const
    -> Semantic::Negotiation::Binding::Status {
  return id == Unknown::contract_id
             ? Semantic::Negotiation::Binding::Status::Satisfied
             : Semantic::Negotiation::Binding::Status::Pending;
}

auto Unknown::bind_interface(System::Uuid id, Data::Form::Storage requested)
    const -> Semantic::Negotiation::Binding::Status {
  if (id == Unknown::contract_id) {
    return Semantic::Negotiation::Binding::marker(requested);
  }

  return Semantic::Negotiation::Binding::Status::Pending;
}

auto Unknown::resolve_concept(Core::View::Bytes) const -> Abstract {
  return get_unknown();
}

auto Unknown::get_unknown() -> Abstract {
  static const Unknown subject;
  return Abstract::provide(subject);
}

PERIMORTEM_C ttx_abstract ttx_unknown(void) {
  return Unknown::get_unknown().get_abi();
}
