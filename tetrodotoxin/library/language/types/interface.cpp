// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/interface.hpp"

#include "tetrodotoxin/library/language/field.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;

static constexpr Tetrodotoxin::Source::Layouts::Named interface_layout;

auto Types::Interface::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Interface& {
  return domain.construct_from<Interface>(
      [&]() -> Interface { return Interface(domain, definition); });
}

auto Types::Interface::create_restored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Interface& {
  return create_authored(domain, definition);
}

auto Types::Interface::create_default(Allocator::Arena&) const
    -> Perimortem::Core::Option<Model::Pack&> {
  return {};
}

auto Types::Interface::create_fitted(Allocator::Arena&, Model::Pack&) const
    -> Perimortem::Core::Option<Model::Pack&> {
  return {};
}

auto Types::Interface::get_layout() const -> const Tetrodotoxin::Source::Layouts::Named& {
  return interface_layout;
}

auto Types::Interface::get_state_layout() const
    -> const Tetrodotoxin::Source::Layouts::Named& {
  return Structure::get_layout();
}

auto Types::Interface::retain_binding(
    Tetrodotoxin::Source::Abstract& binding,
    Tetrodotoxin::Language::Definition& definition,
    Category category,
    Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool {
  auto field = binding.select<Field>();
  if (category != Category::Addressable || !field ||
      field->get_writability() != Writability::Internal ||
      definition.get_visibility() !=
          Tetrodotoxin::Language::Visibility::Public ||
      !field->get_type_reference()) {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Library Interface members require public state with an explicit Type."_view,
        "Declare the shared state as `public state name : Type`."_view);
    return False;
  }

  return Structure::retain_binding(binding, definition, category, cursor);
}
