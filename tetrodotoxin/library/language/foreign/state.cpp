// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/foreign.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

auto Language::Foreign::State::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    TypeReference type_reference,
    View::Bytes abi) -> State& {
  return create(domain, definition, type_reference, abi);
}

auto Language::Foreign::State::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    TypeReference type_reference,
    View::Bytes abi) -> State& {
  return domain.construct_from<State>(
      [&]() -> State { return State(definition, type_reference, abi); });
}

auto Language::Foreign::State::link(Cursor& cursor) -> Bool {
  // Foreign forwards unchanged Type names to its Source. State can therefore
  // retain its real declaration host without acquiring Source internals.
  auto selected =
      type_reference.resolve_authored(cursor, definition.get_host());
  BAIL_IF(!selected);
  auto selected_type = selected->select<Language::Model::Type>();
  if (!selected_type) {
    cursor.create_expression_error(
        type_reference.get_anchor(),
        "Foreign State Type route did not resolve to one stable Type."_view,
        "Publish the exact Type in this Library source before linking."_view);
    return False;
  }
  if (selected_type->get_layout().is_empty()) {
    cursor.create_expression_error(
        type_reference.get_anchor(),
        "Foreign State cannot bind an empty Type Layout."_view,
        "Choose a Type that supplies at least one value."_view);
    return False;
  }

  if (type && &type->get() != &*selected_type) {
    cursor.create_expression_error(
        type_reference.get_anchor(),
        "Foreign State cannot change its linked Type identity."_view,
        "Repeat completion with the original resolved Type."_view);
    return False;
  }

  type = Reference<const Language::Model::Type>(*selected_type);
  return True;
}

auto Language::Foreign::State::link_restored_declaration_type() -> Bool {
  Option<const Model::Type&> selected_type;
  type_reference.resolve_lexical(*this).visit(
      [&](const Abstract& selected) {
        selected_type = selected.select<Model::Type>();
      },
      [](const TypeReference::Failure&) {});
  BAIL_IF(!selected_type || selected_type->get_layout().is_empty());
  type = Reference<const Model::Type>(*selected_type);
  return True;
}

auto Language::Foreign::State::get_type() const -> const Abstract& {
  if (type) {
    return type->get();
  }

  Option<const Abstract&> selected;
  type_reference.resolve_lexical(definition.get_host())
      .visit(
          [&](const Abstract& answer) { selected = answer; },
          [](const TypeReference::Failure&) {});
  auto selected_type = selected ? selected->select<Language::Model::Type>()
                                : Option<const Language::Model::Type&>();
  return selected_type ? static_cast<const Abstract&>(*selected_type)
                       : static_cast<const Abstract&>(Unknown::get_unknown());
}

auto Language::Foreign::State::resolve() const -> const Abstract& {
  return type ? static_cast<const Abstract&>(*this) : Unknown::get_unknown();
}
