// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/unwrap.hpp"

#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

static auto select_option_constant(Language::Model::Pack& source)
    -> Core::Option<Language::Constants::Option&> {
  auto direct = source.select_identity<Language::Constants::Option>();
  if (direct) {
    return *direct;
  }

  const Layout& layout = source.get_layout();
  BAIL_IF(layout.get_size() != 1);
  return layout.get_abstract(0).visit(
      []() -> Core::Option<Language::Constants::Option&> { return {}; },
      [](const Abstract& selected)
          -> Core::Option<Language::Constants::Option&> {
        auto pack =
            Language::Model::Pack::from(const_cast<Abstract&>(selected));
        return pack ? pack->select_identity<Language::Constants::Option>()
                    : Core::Option<Language::Constants::Option&>();
      });
}

auto Language::Access::Unwrap::create_authored(
    Memory::Allocator::Arena& domain,
    Model::Pack& receiver,
    Anchor anchor) -> Unwrap& {
  return Expression::create_authored<Unwrap>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Unwrap {
        return Unwrap(receiver, source);
      });
}

auto Language::Access::Unwrap::link(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));
  auto option = receiver.get_type().resolve().select<Types::Option>();
  if (!option) {
    cursor.create_expression_error(
        get_anchor(), "Postfix `!` requires one Option value."_view,
        "Use `!` only where absence should produce the element Type default."_view);
    return False;
  }

  if (element_type && &element_type->get() != &option->get_element_type()) {
    cursor.create_expression_error(
        get_anchor(), "Option unwrap selected a different element Type."_view,
        "Repeat linking with the same completed Option identity."_view);
    return False;
  }

  element_type =
      Reference<const Language::Model::Type>(option->get_element_type());
  // Absence uses the element Type's ordinary default protocol. Retaining that
  // Pack here makes later folding independent from mutable declaration state
  // without teaching Unwrap how any concrete Type constructs its value.
  auto selected_fallback =
      option->get_element_type().create_default(cursor.get_arena());
  BAIL_IF(!selected_fallback);
  fallback = Tetrodotoxin::Source::PackReference<Model::Pack>(*selected_fallback);
  return Expression::link(cursor, lexical_context, access_scope);
}

auto Language::Access::Unwrap::get_type() const -> const Abstract& {
  return element_type.visit(
      []() -> const Abstract& { return Unknown::get_unknown(); },
      [](const Reference<const Language::Model::Type>& selected)
          -> const Abstract& { return selected.get(); });
}

auto Language::Access::Unwrap::finalize(Cursor& cursor) -> void {
  receiver.finalize(cursor);
  Expression::finalize(cursor);
}

auto Language::Access::Unwrap::evaluate()
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

  auto option = select_option_constant(*folded);
  if (!option || !element_type) {
    return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
  }

  auto payload = option->get_payload();
  if (payload) {
    return const_cast<Model::Pack&>(*payload);
  }

  if (!fallback) {
    return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
  }

  Model::Pack& selected_fallback = fallback->get();
  // A dynamic default remains valid authored flow but cannot become a folded
  // result. Every entry must prove Constant before this operation publishes it.
  const Layout& layout = selected_fallback.get_layout();
  for (Count index = 0; index < layout.get_size(); index++) {
    auto value = layout.get_abstract(index);
    if (!value || !value->is<Constant>()) {
      return Core::Option<Model::Pack&>{};
    }
  }
  return selected_fallback;
}
