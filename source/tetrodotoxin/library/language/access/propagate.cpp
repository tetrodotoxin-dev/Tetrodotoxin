// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/propagate.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/flow/scope.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

auto Language::Access::Propagate::create_authored(
    Memory::Allocator::Arena& domain,
    Model::Pack& receiver,
    Anchor anchor) -> Propagate& {
  Model::Pack& empty_escape = Model::Pack::create_empty(domain);
  return Expression::create_authored<Propagate>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Propagate {
        return Propagate(receiver, empty_escape, source);
      });
}

auto Language::Access::Propagate::link(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));
  auto selected_type = receiver.get_type().resolve().select<Model::Type>();
  auto propagated = selected_type ? selected_type->get_propagated_type()
                                  : Core::Option<const Model::Type&>();
  if (!selected_type || !propagated) {
    cursor.create_expression_error(
        get_anchor(), "Postfix `?` requires a propagating value Type."_view,
        "Use Option, Bool, Result, or another Type that defines propagation."_view);
    return False;
  }

  auto scope = lexical_context.select<Flow::Scope>();
  if (receiver_type && &receiver_type->get() != &*selected_type) {
    cursor.create_expression_error(
        get_anchor(), "Postfix `?` selected a different receiver Type."_view,
        "Repeat linking with the same completed receiver identity."_view);
    return False;
  }

  auto propagated_error = selected_type->get_propagated_error_type();
  if (propagated_error) {
    if (error_type && &error_type->get() != &*propagated_error) {
      cursor.create_expression_error(
          get_anchor(),
          "Postfix `?` selected a different propagated error Type."_view,
          "Repeat linking with the same completed receiver Type."_view);
      return False;
    }

    if (!error_type) {
      ErrorEscape& created = Expression::create_synthetic<ErrorEscape>(
          cursor.get_arena(),
          [&](Core::Option<Anchor>) { return ErrorEscape(*propagated_error); });
      escape = Tetrodotoxin::Source::PackReference<Model::Pack>(created);
      error_type =
          Tetrodotoxin::Source::Reference<const Model::Type>(*propagated_error);
    }
  } else if (error_type) {
    cursor.create_expression_error(
        get_anchor(), "Postfix `?` changed its propagated escape shape."_view,
        "Repeat linking with the same completed receiver Type."_view);
    return False;
  }

  Model::Pack& selected_escape = escape.get();
  BAIL_IF(!selected_escape.link(cursor, lexical_context, access_scope));
  if (!scope || !selected_escape.fits(scope->get_function_results())) {
    auto report = cursor.create_report(get_anchor());
    report
        << "Postfix `?` escape values do not fit the Function result Layout.\n"
           "Escape produces: "_view;
    Language::Diagnostics::write_pack(report, selected_escape);
    report << "\nFunction accepts: "_view;
    if (scope) {
      Language::Diagnostics::write_layout(
          report, scope->get_function_results());
    } else {
      report << "<no Function scope>"_view;
    }
    if (error_type) {
      report.get_hint()
          << "Return the propagated error Type or a receiving Result with that "
             "exact error Type."_view;
    } else {
      report.get_hint()
          << "Use an empty Function result or one receiving Option result."_view;
    }
    return False;
  }

  if (continuation_type && &continuation_type->get() != &*propagated) {
    cursor.create_expression_error(
        get_anchor(),
        "Postfix `?` selected a different continuation Type."_view,
        "Repeat linking with the same completed receiver identity."_view);
    return False;
  }

  receiver_type = Reference<const Language::Model::Type>(*selected_type);
  continuation_type = Reference<const Language::Model::Type>(*propagated);
  return Expression::link(cursor, lexical_context, access_scope);
}

auto Language::Access::Propagate::get_type() const -> const Abstract& {
  return continuation_type.visit(
      []() -> const Abstract& { return Unknown::get_unknown(); },
      [](const Reference<const Language::Model::Type>& selected)
          -> const Abstract& { return selected.get(); });
}

auto Language::Access::Propagate::finalize(Cursor& cursor) -> void {
  receiver.finalize(cursor);
  escape.get().finalize(cursor);
  Expression::finalize(cursor);
}

auto Language::Access::Propagate::evaluate()
    -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
  Core::Option<Model::Pack&> folded;
  Core::Option<Expression::Error> error;
  Expression::fold(receiver).visit(
      [&](const Core::Option<Model::Pack&>& selected) { folded = selected; },
      [&](const Expression::Error& selected) { error = selected; });
  if (error) {
    return *error;
  }
  if (!folded) {
    return Core::Option<Model::Pack&>{};
  }

  auto selected_type = receiver_type.visit(
      []() -> Core::Option<const Language::Model::Type&> { return {}; },
      [](const Reference<const Language::Model::Type>& selected)
          -> Core::Option<const Language::Model::Type&> {
        return selected.get();
      });
  if (!selected_type) {
    return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
  }

  return selected_type->fold_propagation(*folded).visit(
      [](const Core::Option<Model::Pack&>& propagated)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        return propagated;
      },
      [&](Bool)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        return Expression::Error(
            Expression::Error::Type::InvalidConstant, *this);
      });
}
