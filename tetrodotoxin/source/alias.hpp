// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/answers/none.hpp"
#include "ttx/concept/answers/unknown.hpp"

namespace Tetrodotoxin::Source {

// Alias makes one required referent shareable before its identity is known.
// Every semantic question goes to that referent. The retained value is the
// complete borrowed Abstract, so a foreign subject needs no native wrapper.
// Names and policy belong to the supplying declaration rather than to Alias.
//
// Commitment explicitly resolves its candidate once. None cannot fulfill the
// required relationship, and a different answer cannot replace a commitment.
// The graph owner keeps both the referent and this publication alive.
class Alias {
  using Abstract = Ttx::Concept::Abstract;
  using Visitor = Abstract::Visitor;

 public:
  Alias() : target(Ttx::Concept::Answers::Unknown::get_unknown()) {}
  Alias(const Alias&) = delete;
  Alias(Alias&&) = delete;

  auto commit(Abstract subject) -> Bool {
    const auto selected = subject.resolve();
    const Bool absent = selected.supports<Ttx::Concept::Answers::None>() ==
                        Ttx::Semantic::Negotiation::Binding::Status::Satisfied;
    if (selected == Abstract::provide(*this) || absent ||
        selected.resolve() != selected) {
      return False;
    }

    const Bool pending = target.supports<Ttx::Concept::Answers::Unknown>() ==
                         Ttx::Semantic::Negotiation::Binding::Status::Satisfied;
    if (!pending) {
      return target == selected;
    }

    target = selected;
    return True;
  }

  auto get_data() const -> Perimortem::Core::View::Bytes {
    const auto current = target;
    return current.get_data();
  }

  auto resolve() const -> Abstract { return target; }

  auto resolve_concept(Perimortem::Core::View::Bytes route) const -> Abstract {
    const auto current = target;
    return current.resolve_concept(route);
  }

  auto visit_concepts(Visitor visitor) const -> void {
    const auto current = target;
    current.visit_concepts(visitor);
  }

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    return target.supports(id);
  }

  auto bind_interface(
      Perimortem::System::Uuid requested,
      Ttx::Data::Form::Storage destination) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    const auto current = target;
    return current.bind_interface(requested, destination);
  }

 private:
  Abstract target;
};

}  // namespace Tetrodotoxin::Source
