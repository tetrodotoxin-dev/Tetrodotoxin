// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/range_loop.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

Language::Flow::RangeLoop::RangeLoop(
    Allocator::Arena& domain,
    Block& lexical_context,
    View::Vector<AuthoredBinding> source_bindings,
    Language::Model::Pack& input,
    Anchor anchor)
    : domain(domain),
      lexical_context(lexical_context),
      authored_bindings(domain),
      bindings(domain),
      binding_entries(domain),
      input(input),
      anchor(anchor) {
  authored_bindings.reset(source_bindings.get_size());
  bindings.reset(source_bindings.get_size());
  binding_entries.reset(source_bindings.get_size());
  for (const AuthoredBinding& binding : source_bindings) {
    authored_bindings.insert(binding);
  }
}

auto Language::Flow::RangeLoop::create_authored(
    Allocator::Arena& domain,
    Block& lexical_context,
    View::Vector<AuthoredBinding> bindings,
    Language::Model::Pack& input,
    Anchor anchor) -> RangeLoop& {
  return domain.construct_from<RangeLoop>([&]() -> RangeLoop {
    return RangeLoop(domain, lexical_context, bindings, input, anchor);
  });
}

auto Language::Flow::RangeLoop::complete_body(
    Block& selected,
    Anchor selected_anchor) -> Bool {
  if (body) {
    return &body->get() == &selected;
  }
  body = Reference<Block>(selected);
  anchor = selected_anchor;
  return True;
}

auto Language::Flow::RangeLoop::link(
    Cursor& cursor,
    const Language::Model::Type& access_scope) -> Bool {
  if (linked) {
    return True;
  }
  BAIL_IF(!body);

  Managed::Vector<Reference<const Language::Model::Type>> selected_types(
      domain);
  selected_types.reset(authored_bindings.get_size());
  for (const AuthoredBinding& binding : authored_bindings.get_view()) {
    const Abstract& shadowed = lexical_context.resolve_concept(binding.name);
    if (!shadowed.is<Unknown>() && !shadowed.is<None>()) {
      auto report =
          cursor.create_report(Anchor::create(Span(binding.name_token)));
      report << "Library for binding shadows a reachable lexical binding."_view;
      auto& note = report.get_hint();
      note << "Rename this binding so every enclosing name remains "
              "unambiguous."_view;
      auto original = cursor.get_associations().find(shadowed);
      if (original) {
        Token focus = original->get_token();
        if (!focus) {
          focus = original->get_span().get_start();
        }
        if (focus) {
          note << " Original declaration: "_view << cursor.get_source_path()
               << ":"_view << focus.get_line() << ":"_view << focus.get_column()
               << "."_view;
        }
      }
      return False;
    }

    auto selected =
        binding.type_reference.resolve_authored(cursor, lexical_context);
    BAIL_IF(!selected);
    auto selected_type = selected->select<Language::Model::Type>();
    if (!selected_type || selected_type->get_layout().is_empty()) {
      cursor.create_expression_error(
          binding.type_reference.get_anchor(),
          "For loop binding did not resolve to one nonempty Type."_view,
          "Use one completed value Type for each loop binding."_view);
      return False;
    }

    selected_types.insert(*selected_type);
  }

  if (!binding_layout) {
    for (Count index = 0; index < authored_bindings.get_size(); index++) {
      const AuthoredBinding& source = authored_bindings[index];
      auto binding = Tetrodotoxin::Source::Layouts::Addressable::create_authored(
          domain, source.name, selected_types[index].get());
      BAIL_IF(!binding);
      bindings.insert(*binding);
      binding_entries.insert(*binding);
      cursor.get_associations().create(
          Anchor::create(Span(source.name_token)), *binding);
    }
    binding_layout = Tetrodotoxin::Source::Layouts::Named(binding_entries.get_view());
  } else {
    BAIL_IF(bindings.get_size() != selected_types.get_size());
    for (Count index = 0; index < bindings.get_size(); index++) {
      BAIL_IF(
          &bindings[index].get().get_type() != &selected_types[index].get());
    }
  }

  Language::Model::Pack& retained_input = input.get();
  BAIL_IF(!retained_input.link(cursor, lexical_context, access_scope));

  Option<const Language::Model::Type&> selected_input;
  const Abstract& value_type = retained_input.get_type().resolve();
  auto direct_type = value_type.select<Language::Model::Type>();
  if (direct_type) {
    selected_input = *direct_type;
  } else {
    auto expression = retained_input.select_identity<Language::Expression>();
    if (expression) {
      selected_input =
          expression->get_result().resolve().select<Language::Model::Type>();
    }
  }

  if (!selected_input || !selected_input->accepts_iteration(*binding_layout)) {
    cursor.create_expression_error(
        anchor,
        "For loop input cannot produce the authored binding Layout."_view,
        "Match the binding names and Types to one iterable input Type."_view);
    return False;
  }

  if (input_type && &input_type->get() != &*selected_input) {
    cursor.create_expression_error(
        anchor,
        "For loop input selected a different iterable Type identity."_view,
        "Repeat linking with the same completed declaration graph."_view);
    return False;
  }

  input_type = Reference<const Language::Model::Type>(*selected_input);
  BAIL_IF(!body->get().link(cursor));

  linked = True;
  return True;
}

auto Language::Flow::RangeLoop::finalize(Cursor& cursor) -> void {
  input.get().finalize(cursor);
  body.visit(
      []() {},
      [&](Reference<Block>& selected) { selected.get().finalize(cursor); });
}

auto Language::Flow::RangeLoop::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  for (const Reference<Tetrodotoxin::Source::Layouts::Addressable>& binding :
       bindings.get_view()) {
    if (binding.get().get_name() == route) {
      return binding.get();
    }
  }

  return lexical_context.resolve_concept(route);
}

auto Language::Flow::RangeLoop::resolve_authored_context(
    View::Bytes route,
    Count offset) const -> const Abstract& {
  for (const Reference<Tetrodotoxin::Source::Layouts::Addressable>& binding :
       bindings.get_view()) {
    if (binding.get().get_name() == route) {
      return binding.get();
    }
  }

  return lexical_context.resolve_authored_context(route, offset);
}
