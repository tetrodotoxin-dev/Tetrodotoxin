// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operation.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto retain_inputs(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Tetrodotoxin::Source::PackReference<Language::Model::Pack>>
        expressions)
    -> Memory::Managed::Vector<
        Tetrodotoxin::Source::PackReference<Language::Model::Pack>> {
  // Operation owns the mutable traversal inventory. Packs borrow the
  // completed view below. No operand edge is copied into another graph.
  Memory::Managed::Vector<Tetrodotoxin::Source::PackReference<Language::Model::Pack>>
      retained(domain);
  retained.reset(expressions.get_size());
  for (const auto& expression : expressions) {
    retained.insert(expression);
  }
  return retained;
}

Language::Operation::Operation(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>> expressions,
    Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
    : Expression(anchor),
      domain(domain),
      inputs(retain_inputs(domain, expressions)) {}

auto Language::Operation::get_type() const -> const Tetrodotoxin::Source::Abstract& {
  return result_type.visit(
      []() -> const Tetrodotoxin::Source::Abstract& {
        return Tetrodotoxin::Source::Unknown::get_unknown();
      },
      [](const Tetrodotoxin::Source::Reference<const Language::Model::Type>& selected)
          -> const Tetrodotoxin::Source::Abstract& { return selected.get(); });
}

auto Language::Operation::link(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Tetrodotoxin::Source::Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  Bool failed = False;
  auto source_anchor = get_anchor();

  // Child order is authored evaluation order. Independent failures continue
  // so diagnostics retain that same order without making later graph edges
  // disappear from the source model.
  for (Count i = 0; i < inputs.get_size(); i++) {
    Model::Pack& input = inputs[i].get();

    // Operations preserve their caller's two contexts unchanged. Operand
    // nesting changes evaluation order, not lexical shadowing or host access.
    failed |= !input.link(cursor, lexical_context, access_scope);
  }

  BAIL_IF(failed);

  auto selected = select_type(lexical_context);
  if (!selected) {
    auto report = cursor.create_report(source_anchor);
    report << "Operation '"_view << get_name()
           << "' rejects operand Types ["_view;
    for (Count index = 0; index < inputs.get_size(); index++) {
      if (index != 0) {
        report << ", "_view;
      }
      Language::Diagnostics::write_type(
          report, inputs.at(index).get().get_type());
    }
    report << "]."_view;
    report.get_hint()
        << "Use the exact matching operand Types accepted by '"_view
        << get_name() << "'."_view;
    return False;
  }

  if (result_type) {
    if (&result_type->get() == &*selected) {
      return True;
    }

    auto report = cursor.create_report(source_anchor);
    report << "Internal semantic error: Operation '"_view << get_name()
           << "' changed result Type from '"_view
           << result_type->get().get_name() << "' to '"_view
           << selected->get_name() << "'."_view;
    report.get_hint()
        << "The source is valid; report this unstable linking result."_view;
    return False;
  }

  result_type = Tetrodotoxin::Source::Reference<const Language::Model::Type>(*selected);
  return True;
}

auto Language::Operation::finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void {
  // Operands are the canonical authored evaluation inventory. Finalize each
  // real producer in source order before asking this operation to cache its
  // own optional folded result.
  for (Tetrodotoxin::Source::PackReference<Model::Pack> input : inputs.get_view()) {
    input.get().finalize(cursor);
  }
  Expression::finalize(cursor);
}

auto Language::Operation::evaluate()
    -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
  Bool all_reached_folded = True;
  for (Count i = 0; i < inputs.get_size(); i++) {
    auto child_result = fold_input(i);
    Core::Option<Constant&> child_fold;
    Core::Option<Expression::Error> child_error;
    child_result.visit(
        [&](const Core::Option<Constant&>& selected) { child_fold = selected; },
        [&](const Expression::Error& error) { child_error = error; });
    if (child_error) {
      return *child_error;
    }

    all_reached_folded &= bool(child_fold);
    if (i + 1 < inputs.get_size() && child_fold &&
        !reaches_next_input(i, *child_fold)) {
      break;
    }
  }

  if (!all_reached_folded) {
    return Core::Option<Model::Pack&>{};
  }

  return evaluate_constants(domain).visit(
      [](const Core::Option<Tetrodotoxin::Library::Language::Constant&>&
             constant)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        return constant.visit(
            []() -> Core::Option<Model::Pack&> { return {}; },
            [](Tetrodotoxin::Library::Language::Constant& selected)
                -> Core::Option<Model::Pack&> { return selected; });
      },
      [](const Expression::Error& error)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        return error;
      });
}

auto Language::Operation::reaches_next_input(Count, const Constant&) const
    -> Bool {
  return True;
}

auto Language::Operation::fold_input(Count index)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  if (index >= inputs.get_size()) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  Model::Pack& input = inputs[index].get();
  auto constant = input.select_identity<Constant>();
  if (constant) {
    return *constant;
  }

  auto expression = input.select_identity<Expression>();
  if (!expression) {
    return Expression::Error::from_pack(
        Expression::Error::Type::InvalidInput, input);
  }

  return expression->fold().visit(
      [](const Core::Option<Model::Pack&>& folded)
          -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
        return folded.visit(
            []()
                -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
              return Core::Option<Constant&>{};
            },
            [](Model::Pack& selected)
                -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
              auto constant = selected.select_identity<Constant>();
              return constant ? Core::Option<Constant&>(*constant)
                              : Core::Option<Constant&>{};
            });
      },
      [](const Expression::Error& error)
          -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
        return error;
      });
}

auto Language::Operation::get_folded_input(Count index)
    -> Core::Option<Constant&> {
  BAIL_IF(index >= inputs.get_size());

  Model::Pack& input = inputs[index].get();
  auto constant = input.select_identity<Constant>();
  if (constant) {
    return *constant;
  }
  auto expression = input.select_identity<Expression>();
  BAIL_IF(!expression);
  auto folded = expression->get_folded();
  BAIL_IF(!folded);
  return folded->select_identity<Constant>();
}
