// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialect/library/function.hpp"
#include "ttx/semantic/ownership/publication.hpp"

namespace Tetrodotoxin::Dialect::Library {

// A Monograph retains the results of one invocation of this dialect. The
// current Library grammar produces one Function, whose authored policy remains
// visible through this owner. That function can belong to a larger invocation;
// neither this Monograph nor its allocation domain claims to be the source
// root.
//
// Publication transfers the owner's lifetime while Abstract supplies its
// portable observations. An enclosing dialect can retain that Publication as
// a child, expose the child through its own policy, or forward the Publication
// directly. It needs no knowledge of this Arena or native implementation.
// Source observations, context and executable code keep their borrowed lifetime
// contracts. The input Cursor itself is no longer needed after interpretation.
class Monograph {
 public:
  static auto interpret(
      Source::Lexical::Cursor& cursor,
      Ttx::Concept::Abstract context,
      Perimortem::System::Uuid operation)
      -> Perimortem::Utility::Result<
          Ttx::Semantic::Ownership::Publication,
          Ttx::Semantic::Negotiation::Binding::Failure>;

  auto get_data() const -> Perimortem::Core::View::Bytes;
  auto supports(Perimortem::System::Uuid contract) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid contract,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> Ttx::Concept::Abstract;
  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void;

 private:
  // Only successful interpretation publishes this owner. Keeping construction
  // local also gives rejection one cleanup path for the unpublished graph.
  Monograph() = default;

  Perimortem::Memory::Allocator::Arena arena;
  const Function* function = nullptr;
};

}  // namespace Tetrodotoxin::Dialect::Library
