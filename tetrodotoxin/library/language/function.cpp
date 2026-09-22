// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/function.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

auto Language::Function::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Signature& signature) -> Function& {
  return create(domain, definition, signature);
}

auto Language::Function::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Signature& signature) -> Function& {
  return domain.construct_from<Function>(
      [&]() -> Function { return Function(definition, signature); });
}

auto Language::Function::complete_body(Flow::Block& selected) -> Bool {
  if (body) {
    return &*body == &selected;
  }

  body = selected;
  return True;
}

Language::Function::Function(
    Tetrodotoxin::Language::Definition& definition,
    Signature& signature)
    : definition(definition), signature(signature) {}

auto Language::Function::link_declaration_signature(Cursor& cursor) -> Bool {
  if (is_signature_linked()) {
    return True;
  }

  // Signature routes receive the host Type directly. They therefore use the
  // same access authority as the body without making an incomplete Function
  // double as a Type resolution mode switch.
  return signature.link(cursor);
}

auto Language::Function::link_restored_declaration_signature() -> Bool {
  return signature.link_restored();
}

auto Language::Function::link_declaration_body(Cursor& cursor) -> Bool {
  BAIL_IF(!is_signature_linked());
  BAIL_IF(!body);

  // Signature edges publish before Block linking so every Identifier can reach
  // the exact Layout-owned Addressable for its authored parameter slot.
  BAIL_IF(!body->link(cursor));
  if (!get_results().is_empty() && !get_self_result() &&
      body->reaches_next_statement()) {
    cursor.create_expression_error(
        body->get_anchor(),
        "Function result Layout requires a terminal return statement."_view,
        "Return the complete ordered values required by the Function "
        "signature."_view);
    return False;
  }

  return True;
}

auto Language::Function::finalize_declaration(Cursor& cursor) -> Bool {
  BAIL_IF(!body);

  Bool valid = True;
  if (get_definition().is_published()) {
    valid = signature.validate_publication(cursor);
  }

  // Optional folding records a cached Constant for later consumers. A dynamic
  // result or failure remains queryable but cannot turn an otherwise complete
  // Function into a semantic failure without a Constant requirement.
  body->finalize(cursor);

  return valid;
}

auto Language::Function::resolve() const -> const Abstract& {
  if (!is_signature_linked()) {
    return Unknown::get_unknown();
  }

  return *this;
}

auto Language::Function::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  const Abstract& parameter =
      signature.get_parameters().resolve_named(route, get_host());
  if (&parameter != &Unknown::get_unknown()) {
    return parameter;
  }

  return get_host().resolve_lexical_context(route);
}

auto Language::Function::get_parameters() const -> const Layout& {
  // Callable Layouts are total only after resolve() proves this Function's
  // Signature. Returning an empty Layout here would launder incomplete state
  // into valid zero value flow, so the lifecycle precondition remains explicit.
  return signature.get_parameters();
}

auto Language::Function::get_results() const -> const Layout& {
  return signature.get_results();
}

auto Language::Function::declares_self() const -> Bool {
  return signature.declares_self();
}

auto Language::Function::get_body() const -> Option<const Flow::Block&> {
  return body.visit(
      []() -> Option<const Flow::Block&> { return {}; },
      [](const Flow::Block& selected) -> Option<const Flow::Block&> {
        return selected;
      });
}

auto Language::Function::is_signature_linked() const -> Bool {
  return signature.is_linked();
}
