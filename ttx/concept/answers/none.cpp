// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/answers/none.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/answers/constant.hpp"

using namespace Ttx;
using namespace Perimortem;

// Completed absence stays absent along every named route. Keeping ordinary
// navigation available lets a consumer continue its walk without inventing a
// value, while the marker itself needs no borrowed operations.
class NoneProvider {
 public:
  auto get_data() const -> Core::View::Bytes { return "None"_view; }
  auto bind_interface(System::Uuid id, Data::Form::Storage requested) const
      -> Semantic::Negotiation::Binding::Status {
    if (id == Concept::Answers::None::contract_id ||
        id == Concept::Answers::Constant::contract_id) {
      return Semantic::Negotiation::Binding::marker(requested);
    }

    return Semantic::Negotiation::Binding::Status::Unsupported;
  }

  auto resolve_concept(Core::View::Bytes) const -> Concept::Abstract {
    return Concept::Abstract::provide(*this);
  }
};

PERIMORTEM_C ttx_abstract ttx_none(void) {
  static const NoneProvider provider;
  return Concept::Abstract::provide(provider).get_abi();
}

auto Concept::Answers::None::get_none() -> Abstract {
  return Abstract(ttx_none());
}
