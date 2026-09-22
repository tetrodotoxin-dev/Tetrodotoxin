// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/subtract_assignment.hpp"

#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/source/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

static constexpr Tetrodotoxin::Source::Layouts::Fluid assignment_layout;

auto Language::Operations::SubtractAssignment::create_authored(
    Perimortem::Memory::Allocator::Arena& domain,
    Expression& target,
    Model::Pack& right,
    Anchor anchor) -> SubtractAssignment& {
  return Expression::create_authored<SubtractAssignment>(
      domain, anchor, [&](auto authored) -> SubtractAssignment {
        return SubtractAssignment(target, right, authored);
      });
}

auto Language::Operations::SubtractAssignment::link(
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
        get_anchor(),
        "Subtraction assignment requires one Library Type authority."_view,
        "Retain the enclosing Function host while linking this expression."_view);
    return False;
  }

  // Scalar compound assignment asks the same target operation to bind its
  // write edge while retaining the target Type for the required read.
  BAIL_IF(!target.link_write(cursor, lexical_context, *selected_scope, right));

  auto target_type = target.get_write_type(*selected_scope);
  const Abstract& read_type = target.get_type().resolve();
  const Abstract& right_type = right.get_type().resolve();
  Bool numeric = target_type && (target_type->is<Model::Types::Unsigned>() ||
                                 target_type->is<Model::Types::Signed>() ||
                                 target_type->is<Model::Types::Real>());
  if (!target_type || &read_type != &*target_type ||
      &right_type != &*target_type || !numeric) {
    cursor.create_expression_error(
        get_anchor(),
        "Subtraction assignment requires one writable exact numeric Type."_view,
        "Use the same signed, unsigned, or real Type on both sides of `-=`."_view);
    return False;
  }

  linked = True;
  return True;
}

auto Language::Operations::SubtractAssignment::finalize(Cursor& cursor)
    -> void {
  target.finalize(cursor);
  right.finalize(cursor);
}

auto Language::Operations::SubtractAssignment::get_value_type(Count) const
    -> const Abstract& {
  return Unknown::get_unknown();
}

auto Language::Operations::SubtractAssignment::get_layout() const
    -> const Layout& {
  return assignment_layout;
}

auto Language::Operations::SubtractAssignment::resolve() const
    -> const Abstract& {
  return linked ? static_cast<const Expression&>(*this)
                : static_cast<const Abstract&>(Unknown::get_unknown());
}
