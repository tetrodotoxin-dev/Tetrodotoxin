// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/policies/authored.hpp"

namespace Tetrodotoxin::Dialect::Library {

// Library interpretation attaches source policy to an Execution function.
// Types come from the supplied Abstract namespace, which may be provided by a
// different dialect or a loaded module. Parameter lookup belongs to this
// source scope and does not become a requirement on the Execution model.
//
// The grammar accepts named typed parameters, positional results, a single
// parameter or unsigned literal return, and empty bodies. Decimal literals
// must fit U32. Other syntax is diagnosed here, while Type names are resolved
// through the supplied namespace. The caller supplies the operation identity
// associated with this declaration's publication. Its Arena owns the emitted
// native graph independently of the storage used to supply the token stream.
// Declaration Anchors borrow the original Source observation. Names and routes
// are copied into the graph Arena so the supplying Cursor can be released as
// soon as interpretation ends.
class Function {
 public:
  static auto interpret(
      Perimortem::Memory::Allocator::Arena& arena,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Ttx::Concept::Abstract types,
      Perimortem::System::Uuid operation)
      -> Perimortem::Core::Option<Function&>;
  constexpr Function(
      Tetrodotoxin::Source::Policies::Authored declaration,
      Perimortem::Core::View::Vector<Ttx::Concept::Abstract> parameters,
      Ttx::Concept::Abstract body)
      : declaration(declaration), parameters(parameters), body(body) {}
  auto get_data() const -> Perimortem::Core::View::Bytes {
    return declaration.get_data();
  }

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    return declaration.supports(id);
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    return declaration.bind_interface(id, output);
  }

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> Ttx::Concept::Abstract;
  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void;

 private:
  Tetrodotoxin::Source::Policies::Authored declaration;
  Perimortem::Core::View::Vector<Ttx::Concept::Abstract> parameters;
  Ttx::Concept::Abstract body;
};

}  // namespace Tetrodotoxin::Dialect::Library
