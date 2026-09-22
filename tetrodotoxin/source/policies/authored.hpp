// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/declaration.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Source::Policies {

// Authored attaches source evidence to a semantic subject without changing its
// model. Binding provenance is handled here, while other capabilities remain
// governed by the subject being exposed. The source publication retains both
// the evidence and the subject for every borrowed view of this policy.
class Authored {
 public:
  constexpr Authored(
      Ttx::Concept::Abstract subject,
      Tetrodotoxin::Source::Anchor anchor,
      Perimortem::Core::View::Bytes name = {})
      : subject(subject), anchor(anchor), name(name) {}

  auto get_data() const -> Perimortem::Core::View::Bytes { return name; }
  auto get_anchor() const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Anchor> {
    return anchor;
  }

  auto set_anchor(Tetrodotoxin::Source::Anchor value) -> void {
    anchor = value;
  }

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    if (id == Tetrodotoxin::Source::Declaration::contract_id) {
      return Ttx::Semantic::Negotiation::Binding::Status::Satisfied;
    }

    return subject.supports(id);
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    if (id == Tetrodotoxin::Source::Declaration::contract_id) {
      return Tetrodotoxin::Source::Declaration::provide(*this, output);
    }

    return subject.bind_interface(id, output);
  }

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> Ttx::Concept::Abstract {
    return subject.resolve_concept(route);
  }

  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void {
    subject.visit_concepts(visitor);
  }

 private:
  Ttx::Concept::Abstract subject;
  Tetrodotoxin::Source::Anchor anchor;
  Perimortem::Core::View::Bytes name;
};

}  // namespace Tetrodotoxin::Source::Policies
