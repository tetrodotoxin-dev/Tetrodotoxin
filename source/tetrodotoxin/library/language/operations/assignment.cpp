// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/assignment.hpp"

#include "tetrodotoxin/source/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

static constexpr Tetrodotoxin::Source::Layouts::Fluid assignment_layout;

auto Language::Operations::Assignment::create_authored(
    Perimortem::Memory::Allocator::Arena& domain,
    Expression& target,
    Model::Pack& source,
    Anchor anchor) -> Assignment& {
  return Expression::create_authored<Assignment>(
      domain, anchor, [&](auto authored) -> Assignment {
        return Assignment(target, source, authored);
      });
}

auto Language::Operations::Assignment::link(
    Cursor& cursor,
    const Abstract& lexical_context,
    Option<const Abstract&> access_scope) -> Bool {
  if (linked) {
    return True;
  }

  auto selected_scope = access_scope.visit(
      []() -> Option<const Model::Type&> { return {}; },
      [](const Abstract& candidate) -> Option<const Model::Type&> {
        return candidate.select<Model::Type>();
      });
  if (!selected_scope) {
    cursor.create_expression_error(
        get_anchor(), "Assignment requires one Library Type authority."_view,
        "Retain the enclosing Function host while linking this expression."_view);
    return False;
  }

  // The receiving Expression owns target linking and complete Pack admission.
  // Assignment never needs to recover its concrete storage carrier.
  BAIL_IF(!target.link_write(cursor, lexical_context, *selected_scope, source));

  linked = True;
  return True;
}

auto Language::Operations::Assignment::finalize(Cursor& cursor) -> void {
  // Finalization follows the same independent edges fixed during linking.
  target.finalize(cursor);
  source.finalize(cursor);
}

auto Language::Operations::Assignment::get_value_type(Count) const
    -> const Abstract& {
  return Unknown::get_unknown();
}

auto Language::Operations::Assignment::get_layout() const -> const Layout& {
  return assignment_layout;
}

auto Language::Operations::Assignment::resolve() const -> const Abstract& {
  return linked ? static_cast<const Expression&>(*this)
                : static_cast<const Abstract&>(Unknown::get_unknown());
}
