// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/branch.hpp"

#include "tetrodotoxin/library/language/model/types/flag.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

static auto select_condition_flag(const Language::Model::Pack& condition)
    -> Option<const Language::Model::Types::Flag&> {
  return condition.get_value_type(0)
      .resolve()
      .select<Language::Model::Types::Flag>();
}

auto Language::Flow::Branch::create_authored(
    Allocator::Arena& domain,
    Kind kind,
    Model::Pack& condition,
    Anchor anchor) -> Branch& {
  return domain.construct_from<Branch>(
      [&]() -> Branch { return Branch(kind, condition, anchor); });
}

auto Language::Flow::Branch::complete_body(Block& selected) -> Bool {
  if (body) {
    return &body->get() == &selected;
  }
  body = Reference<Block>(selected);
  return True;
}

auto Language::Flow::Branch::complete_alternate(Statement selected) -> Bool {
  if (alternate) {
    return False;
  }
  alternate = selected;
  return True;
}

auto Language::Flow::Branch::complete_anchor(Anchor selected) -> void {
  anchor = selected;
}

auto Language::Flow::Branch::link(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    Scope& lexical_context,
    const Language::Model::Type& access_scope) -> Bool {
  if (linked) {
    return True;
  }
  BAIL_IF(!body);

  Model::Pack& retained_condition = condition.get();
  BAIL_IF(!retained_condition.link(cursor, lexical_context, access_scope));
  // Branch observes value flow rather than the exact identities used by
  // postfix access. Prove that distinction before selecting the leading Flag.
  if (!retained_condition.is_complete()) {
    cursor.create_expression_error(
        anchor, "Branch condition did not produce value flow."_view,
        "Use a Type result only as an access receiver."_view);
    return False;
  }

  if (!select_condition_flag(retained_condition)) {
    cursor.create_expression_error(
        anchor, "Branch condition must produce a Flag as its first value."_view,
        "Keep any additional Pack values after one leading Flag value."_view);
    return False;
  }

  Bool failed = !body->get().link(cursor);
  alternate.visit(
      []() {},
      [&](Statement& selected) {
        failed |= !selected.link(cursor, lexical_context);
      });
  BAIL_IF(failed);

  linked = True;
  return True;
}

auto Language::Flow::Branch::finalize(Cursor& cursor) -> void {
  condition.get().finalize(cursor);
  body.visit(
      []() {},
      [&](Reference<Block>& selected) { selected.get().finalize(cursor); });
  alternate.visit(
      []() {}, [&](Statement& selected) { selected.finalize(cursor); });
}

auto Language::Flow::Branch::reaches_next_statement() const -> Bool {
  if (kind == Kind::While || !alternate) {
    return True;
  }

  return body->get().reaches_next_statement() || alternate->reaches_next();
}
