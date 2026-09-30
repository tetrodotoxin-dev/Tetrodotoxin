// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/model/execution/policies/ordered.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Execution::Statements {

// Block lends an ordered sequence of execution subjects. Normal completion of
// one entry reaches the next, while control transfer such as Return leaves the
// sequence according to that operation's contract. An empty Block falls
// through. The supplying owner retains the array and every borrowed subject.
//
// Ordered qualifies Abstract visitation, so a foreign provider can supply the
// same sequence without a native Block or an additional operations table.
// This provider uses little endian U64 indices as its lookup routes. Consumers
// executing the sequence use visitation order and need not decode those routes.
// Source names, scope and provenance belong to policies around these subjects.
//
// Abstract::provide supplies the base Abstract contract. This leaf adds Ordered
// and leaves other UUIDs Unknown, allowing a composing policy to supply
// its own capabilities without changing the sequence owner.
class Block {
 public:
  explicit constexpr Block(
      Perimortem::Core::View::Vector<Ttx::Concept::Abstract> statements = {})
      : statements(statements) {}

  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> Ttx::Concept::Abstract;
  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void;

 private:
  Perimortem::Core::View::Vector<Ttx::Concept::Abstract> statements;
};

}  // namespace Tetrodotoxin::Model::Execution::Statements
