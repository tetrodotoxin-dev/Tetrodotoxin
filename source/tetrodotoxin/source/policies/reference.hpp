// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/declaration.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Source::Policies {

// A source reference keeps the route and authority that supplied it.
// Each capability request observes that authority again, so an Unknown answer
// can settle without a completion phase or a cached answer that bypasses it.
// This policy self resolves and retains its source location independently of
// the implementation selected by its context. Lexical visibility belongs to
// that context and must reflect the reference position, not a completion pass.
// Context, route bytes and Source observation remain borrowed from their
// supplying publications. This owner retains no selected answer between calls.
class Reference {
 public:
  constexpr Reference(
      Ttx::Concept::Abstract context,
      Perimortem::Core::View::Bytes route,
      Tetrodotoxin::Source::Anchor anchor)
      : context(context), route(route), anchor(anchor) {}
  auto get_data() const -> Perimortem::Core::View::Bytes { return route; }
  auto get_anchor() const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Anchor> {
    return anchor;
  }

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    if (id == Tetrodotoxin::Source::Declaration::contract_id) {
      return Ttx::Semantic::Negotiation::Binding::Status::Satisfied;
    }

    return context.resolve_concept(route).supports(id);
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    if (id == Tetrodotoxin::Source::Declaration::contract_id) {
      return Tetrodotoxin::Source::Declaration::provide(*this, output);
    }

    return context.resolve_concept(route).bind_interface(id, output);
  }

  auto resolve_concept(Perimortem::Core::View::Bytes query) const
      -> Ttx::Concept::Abstract {
    return context.resolve_concept(route).resolve_concept(query);
  }

  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void {
    context.resolve_concept(route).visit_concepts(visitor);
  }

 private:
  Ttx::Concept::Abstract context;
  Perimortem::Core::View::Bytes route;
  Tetrodotoxin::Source::Anchor anchor;
};

}  // namespace Tetrodotoxin::Source::Policies
