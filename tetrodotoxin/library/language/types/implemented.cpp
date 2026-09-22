// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/implemented.hpp"

#include "tetrodotoxin/library/language/interfaces/structure.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library::Language;

auto Types::Implemented::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    TypeReference requirement) -> Implemented& {
  return domain.construct_from<Implemented>([&]() -> Implemented {
    return Implemented(domain, definition, requirement);
  });
}

auto Types::Implemented::create_restored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    TypeReference requirement) -> Implemented& {
  return domain.construct_from<Implemented>([&]() -> Implemented {
    return Implemented(domain, definition, requirement, False);
  });
}

auto Types::Implemented::complete_body() -> void {
  body_complete = True;
  complete_field_layout();
}

auto Types::Implemented::bind_authored_requirement(Tetrodotoxin::Source::Lexical::Cursor& cursor)
    -> Bool {
  if (requirement) {
    return True;
  }

  auto selected = requirement_reference.resolve_authored(cursor, *this);
  auto interface =
      selected ? selected->select<Interface>() : Option<const Interface&>();
  if (!interface) {
    cursor.create_expression_error(
        requirement_reference.get_anchor(),
        "Library implementation requirement did not select an Interface."_view,
        "Choose one completed `interface` Type."_view);
    return False;
  }

  requirement = Reference<const Interface>(*interface);
  return materialize_fields();
}

auto Types::Implemented::resolve_restored_requirement() -> Bool {
  if (requirement) {
    return True;
  }

  Option<const Interface&> selected;
  requirement_reference.resolve_lexical(*this).visit(
      [&](const Abstract& candidate) {
        selected = candidate.select<Interface>();
      },
      [](const TypeReference::Failure&) {});
  BAIL_IF(!selected);
  requirement = Reference<const Interface>(*selected);
  return True;
}

auto Types::Implemented::materialize_fields() -> Bool {
  if (fields_materialized) {
    return True;
  }
  BAIL_IF(!requirement);

  Managed::Vector<Reference<Abstract>> requirements(get_domain());
  for (const Reference<Abstract>& declaration :
       requirement->get().get_addressables(
           Tetrodotoxin::Language::Visibility::Public)) {
    requirements.insert(declaration);
  }
  for (Count index = requirements.get_size(); index != 0; index--) {
    const Reference<Abstract>& declaration = requirements[index - 1];
    auto required = declaration.get().select<Field>();
    BAIL_IF(!required);

    Field* supplied = nullptr;
    for (const Reference<Abstract>& retained : get_addressables()) {
      auto candidate = retained.get().select<Field>();
      if (candidate && candidate->get_name() == required->get_name()) {
        BAIL_IF(supplied);
        supplied = &*candidate;
      }
    }
    if (supplied != nullptr) {
      continue;
    }

    auto& definition = Tetrodotoxin::Language::Definition::create_synthetic(
        get_domain(), required->get_documentation(), *this,
        get_domain().proxy(required->get_name()),
        required->get_definition().get_visibility(),
        Tetrodotoxin::Source::Lexical::Anchor::create(Tetrodotoxin::Source::Lexical::Span()));
    auto type = required->get_type_reference();
    BAIL_IF(!type);
    auto& generated = Field::create_generated(
        get_domain(), definition, required->get_writability(), *type);
    BAIL_IF(!publish_binding(
        generated, Category::Addressable, definition.is_published(), False,
        True));
    generated_fields.emplace(GeneratedField(*required, generated));
  }

  fields_materialized = True;
  return True;
}

auto Types::Implemented::link_fields(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool {
  BAIL_IF(!requirement || !body_complete);
  Interface& selected = const_cast<Interface&>(requirement->get());
  BAIL_IF(!selected.link_fields(cursor));
  complete_field_layout();
  return Object::link_fields(cursor);
}

auto Types::Implemented::link_initializers(Tetrodotoxin::Source::Lexical::Cursor& cursor)
    -> Bool {
  BAIL_IF(!requirement);
  Interface& selected = const_cast<Interface&>(requirement->get());
  BAIL_IF(!selected.link_initializers(cursor));
  for (GeneratedField generated : generated_fields.get_view()) {
    BAIL_IF(!generated.implementation.get().retain_generated_initializer(
        generated.requirement.get()));
  }
  return Object::link_initializers(cursor);
}

auto Types::Implemented::finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool {
  BAIL_IF(!Object::finalize(cursor) || !requirement);
  Interfaces::Structure relation;
  if (!relation.accepts(requirement->get(), *this)) {
    cursor.create_expression_error(
        get_anchor(),
        "Library Object does not satisfy its authored Interface."_view,
        "Match every public Interface Field name, policy, and Type."_view);
    return False;
  }
  return True;
}

auto Types::Implemented::link_restored_fields() -> Bool {
  BAIL_IF(!resolve_restored_requirement());
  Interface& selected = const_cast<Interface&>(requirement->get());
  BAIL_IF(!selected.link_restored_fields() || !materialize_fields());
  complete_field_layout();
  return Object::link_restored_fields();
}

auto Types::Implemented::link_restored_initializers() -> Bool {
  BAIL_IF(!requirement);
  Interface& selected = const_cast<Interface&>(requirement->get());
  BAIL_IF(!selected.link_restored_initializers());
  for (GeneratedField generated : generated_fields.get_view()) {
    BAIL_IF(!generated.implementation.get().retain_generated_initializer(
        generated.requirement.get()));
  }
  return Object::link_restored_initializers();
}

auto Types::Implemented::finalize_restored() -> Bool {
  BAIL_IF(!Object::finalize_restored() || !requirement);
  Interfaces::Structure relation;
  return relation.accepts(requirement->get(), *this);
}

auto Types::Implemented::satisfies(const Abstract& selected) const -> Bool {
  return requirement && &requirement->get().resolve() == &selected.resolve();
}
