// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/object.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library::Language;

static auto select_accessible_field(
    const Abstract& candidate,
    Option<const Abstract&> access_scope) -> Option<const Field&> {
  auto field = candidate.select<Field>();
  BAIL_IF(!field || field->get_writability() != Writability::Internal);

  const Abstract& selected = field->get_host()
                                 .resolve_concept("instance"_view)
                                 .resolve_concept(field->get_name())
                                 .resolve();
  BAIL_IF(&selected != &*field);

  if (field->get_definition().get_visibility() ==
      Tetrodotoxin::Language::Visibility::Private) {
    auto caller = access_scope.visit(
        []() -> Option<const Model::Type&> { return {}; },
        [](const Abstract& selected) {
          return selected.select<Model::Type>();
        });
    BAIL_IF(!caller || !caller->has_private_access_to(field->get_host()));
  }

  // Object construction shares ordinary receiver visibility. This admits
  // published state plus private state reached from a hosted descendant while
  // excluding Static and const Fields from the construction input surface.
  return *field;
}

static auto select_supplied(
    const Field& field,
    Model::Pack& arguments,
    View::Vector<Reference<const Abstract>> fitted_fields)
    -> Option<const Model::Pack&> {
  const Layout& inputs = arguments.get_layout();
  for (Count index = 0; index < fitted_fields.get_size(); index++) {
    if (&fitted_fields.get_data()[index].get() == &field) {
      return inputs.get_abstract(index).visit(
          []() -> Option<const Model::Pack&> { return {}; },
          [](const Abstract& selected) { return Model::Pack::from(selected); });
    }
  }
  return {};
}

static auto fit_supplied_fields(
    Model::Pack& arguments,
    View::Vector<Reference<const Abstract>> accessible_fields,
    Managed::Vector<Reference<const Abstract>>& fitted_fields) -> Bool {
  const Layout& inputs = arguments.get_layout();
  fitted_fields.reset(inputs.get_size());

  for (Count input_index = 0; input_index < inputs.get_size(); input_index++) {
    auto input_name = inputs.get_name(input_index);
    BAIL_IF(!input_name);

    Count selected = 0;
    Count matches = 0;
    for (Count field_index = 0; field_index < accessible_fields.get_size();
         field_index++) {
      if (accessible_fields.get_data()[field_index].get().get_name() ==
          *input_name) {
        selected = field_index;
        matches++;
      }
    }
    BAIL_IF(matches != 1);
    fitted_fields.insert(accessible_fields.get_data()[selected]);
  }

  Layouts::Fluid target_layout(fitted_fields.get_view());
  return arguments.fits(target_layout);
}

Types::Object::Object(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Bool provides_initialization)
    : Structure(domain, definition, provides_initialization) {}

auto Types::Object::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Object& {
  return domain.construct_from<Object>(
      [&]() -> Object { return Object(domain, definition); });
}

auto Types::Object::create_synthetic(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Object& {
  return domain.construct_from<Object>(
      [&]() -> Object { return Object(domain, definition); });
}

auto Types::Object::create_restored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Object& {
  return domain.construct_from<Object>(
      [&]() -> Object { return Object(domain, definition, False); });
}

auto Types::Object::create_default(Allocator::Arena& arena) const
    -> Option<Model::Pack&> {
  // Object owns this override so managed identity cannot become an accidental
  // invariant of inline Structure construction. The retained initialization
  // Pack remains the semantic input to the runtime allocation boundary.
  return Structure::create_default(arena);
}

auto Types::Object::create_supplied(
    Cursor& cursor,
    Model::Pack& arguments,
    Option<const Abstract&> access_scope,
    Option<Anchor> anchor) const -> Option<Model::Pack&> {
  Allocator::Arena& arena = cursor.get_arena();
  Managed::Vector<Reference<const Abstract>> accessible_fields(arena);
  for (const Reference<Abstract>& selected : get_addressables()) {
    auto selected_field = select_accessible_field(selected.get(), access_scope);
    if (selected_field) {
      accessible_fields.insert(*selected_field);
    }
  }

  Managed::Vector<Reference<const Abstract>> fitted_fields(arena);
  // Inputs retain evaluation order, while this fitted Field sequence records
  // which declaration owns each named value. The Pack performs the final fit
  // so receiving Types can admit semantic conversions such as Option payloads.
  if (!fit_supplied_fields(
          arguments, accessible_fields.get_view(), fitted_fields)) {
    cursor.create_expression_error(
        anchor,
        "Object initializer inputs do not fit the initialization Layout."_view,
        "Use unique accessible Fields with values accepted by their Types."_view);
    return {};
  }

  if (!owns_initialization()) {
    return Expressions::Initializer::create_provider(arena, *this, arguments);
  }

  // Object assembles only its owned mutable instance Fields in authored order.
  // A fitted supplied value wins, then the declaration initializer, then the
  // exact Field Type default. Const and Static facts never enter this inventory
  // and therefore cannot become construction inputs by accident.
  Managed::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>> values(arena);
  values.reset(get_layout().get_size());
  for (const Reference<Abstract>& selected : get_addressables()) {
    auto field = selected.get().select<Field>();
    if (!field || field->get_writability() != Writability::Internal) {
      continue;
    }

    auto supplied =
        select_supplied(*field, arguments, fitted_fields.get_view());
    if (supplied) {
      values.insert(const_cast<Model::Pack&>(*supplied));
      continue;
    }

    auto authored = field->get_initializer();
    if (authored) {
      values.insert(const_cast<Model::Pack&>(*authored));
      continue;
    }

    auto field_type = field->get_type().select<Model::Type>();
    auto fallback =
        field_type ? field_type->create_default(arena) : Option<Model::Pack&>();
    if (!fallback) {
      cursor.create_expression_error(
          anchor,
          "Object initializer cannot complete one omitted state Field."_view,
          "Use a completed Field Type with a semantic default or supply an "
          "exact Field value."_view);
      return {};
    }
    values.insert(*fallback);
  }

  return Model::Pack::create_group(arena, values.get_view());
}

auto Types::Object::create_supplied_restored(
    Allocator::Arena& arena,
    Model::Pack& arguments,
    Option<const Abstract&> access_scope) const -> Option<Model::Pack&> {
  Managed::Vector<Reference<const Abstract>> accessible_fields(arena);
  for (const Reference<Abstract>& selected : get_addressables()) {
    auto field = select_accessible_field(selected.get(), access_scope);
    if (field) {
      accessible_fields.insert(*field);
    }
  }
  Managed::Vector<Reference<const Abstract>> fitted_fields(arena);
  BAIL_IF(!fit_supplied_fields(
      arguments, accessible_fields.get_view(), fitted_fields));

  if (!owns_initialization()) {
    return Expressions::Initializer::create_provider(arena, *this, arguments);
  }

  Managed::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>> values(arena);
  values.reset(get_layout().get_size());
  for (const Reference<Abstract>& selected : get_addressables()) {
    auto field = selected.get().select<Field>();
    if (!field || field->get_writability() != Writability::Internal) {
      continue;
    }

    auto supplied =
        select_supplied(*field, arguments, fitted_fields.get_view());
    if (supplied) {
      values.insert(const_cast<Model::Pack&>(*supplied));
      continue;
    }
    auto authored = field->get_initializer();
    if (authored) {
      values.insert(const_cast<Model::Pack&>(*authored));
      continue;
    }
    auto field_type = field->get_type().select<Model::Type>();
    auto fallback =
        field_type ? field_type->create_default(arena) : Option<Model::Pack&>();
    BAIL_IF(!fallback);
    values.insert(*fallback);
  }
  return Model::Pack::create_group(arena, values.get_view());
}
