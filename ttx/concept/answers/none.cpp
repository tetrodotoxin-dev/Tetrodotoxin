// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/answers/none.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/answers/constant.hpp"

using namespace Ttx;
using namespace Perimortem;

using namespace Ttx::Concept;
using namespace Ttx::Concept::Answers;

auto None::get_data() const -> Core::View::Bytes {
  return "None"_view;
}

auto None::supports(System::Uuid id) const
    -> Semantic::Negotiation::Binding::Status {
  return id == None::contract_id || id == Constant::contract_id
             ? Semantic::Negotiation::Binding::Status::Satisfied
             : Semantic::Negotiation::Binding::Status::Unsupported;
}

auto None::bind_interface(System::Uuid id, Data::Form::Storage requested) const
    -> Semantic::Negotiation::Binding::Status {
  if (id == None::contract_id || id == Constant::contract_id) {
    return Semantic::Negotiation::Binding::marker(requested);
  }

  return Semantic::Negotiation::Binding::Status::Unsupported;
}

auto None::resolve_concept(Core::View::Bytes) const -> Abstract {
  return get_none();
}

auto None::get_none() -> Abstract {
  static const None subject;
  return Abstract::provide(subject);
}

PERIMORTEM_C ttx_abstract ttx_none(void) {
  return None::get_none().get_abi();
}
