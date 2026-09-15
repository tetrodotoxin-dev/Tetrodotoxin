// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/answers/unknown.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/answers/constant.hpp"

using namespace Ttx;
using namespace Perimortem;

// An unanswered name remains unanswered. Using a leaf's default None answer
// here would turn missing evidence into completed absence, so both navigation
// and capability requests preserve the provisional answer.
class UnknownProvider {
 public:
  auto get_data() const -> Core::View::Bytes { return "Unknown"_view; }
  auto bind_interface(System::Uuid id, Data::Form::Storage requested) const
      -> Semantic::Negotiation::Binding::Status {
    if (id == Concept::Answers::Unknown::contract_id) {
      return Semantic::Negotiation::Binding::marker(requested);
    }

    return Semantic::Negotiation::Binding::Status::Pending;
  }

  auto resolve_concept(Core::View::Bytes) const -> Concept::Abstract {
    return Concept::Abstract::provide(*this);
  }
};

PERIMORTEM_C ttx_abstract ttx_unknown(void) {
  static const UnknownProvider provider;
  return Concept::Abstract::provide(provider).get_abi();
}

auto Concept::Answers::Unknown::get_unknown() -> Abstract {
  return Abstract(ttx_unknown());
}
