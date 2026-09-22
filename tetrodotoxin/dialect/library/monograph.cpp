// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialect/library/monograph.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Dialect::Library;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Ownership;

auto Monograph::interpret(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    Abstract context,
    System::Uuid operation) -> Utility::Result<Publication, Binding::Failure> {
  auto* monograph = new Monograph();
  const auto parsed =
      Function::interpret(monograph->arena, cursor, context, operation);
  if (!parsed) {
    delete monograph;
    return Binding::Failure::Rejected;
  }

  monograph->function = &*parsed;
  return Publication(
      {Abstract::provide(*monograph).get_query(),
       [](const void* owner) { delete static_cast<const Monograph*>(owner); }});
}

auto Monograph::get_data() const -> Core::View::Bytes {
  return function->get_data();
}

auto Monograph::supports(System::Uuid contract) const -> Binding::Status {
  return function->supports(contract);
}

auto Monograph::bind_interface(
    System::Uuid contract,
    Ttx::Data::Form::Storage output) const -> Binding::Status {
  return function->bind_interface(contract, output);
}

auto Monograph::resolve_concept(Core::View::Bytes route) const -> Abstract {
  return function->resolve_concept(route);
}

auto Monograph::visit_concepts(Abstract::Visitor visitor) const -> void {
  function->visit_concepts(visitor);
}
