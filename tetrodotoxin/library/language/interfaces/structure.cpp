// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/interfaces/structure.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

auto Language::Interfaces::Structure::compatible_type(
    const Abstract& required,
    const Abstract& supplied) -> Bool {
  auto required_type = required.select<Language::Model::Type>();
  auto supplied_type = supplied.select<Language::Model::Type>();
  BAIL_IF(!required_type || !supplied_type);
  const Abstract& required_identity = required_type->resolve();
  const Abstract& supplied_identity = supplied_type->resolve();
  if (&required_identity == &supplied_identity) {
    return True;
  }

  auto required_value =
      required_identity.select<Language::Model::Types::Value>();
  auto supplied_value =
      supplied_identity.select<Language::Model::Types::Value>();
  return required_value && supplied_value &&
         required_value->is_equivalent(*supplied_value);
}

auto Language::Interfaces::Structure::select_field(
    const Language::Field& requirement,
    const Abstract& candidate) const -> Option<const Language::Field&> {
  auto object = candidate.resolve().select<Language::Types::Object>();
  BAIL_IF(!object || !object->is_linked());

  auto selected = object->resolve_concept("instance"_view)
                      .resolve_concept(requirement.get_name())
                      .resolve()
                      .select<Language::Field>();
  BAIL_IF(
      !selected ||
      selected->get_writability() != Language::Writability::Internal ||
      selected->get_definition().get_visibility() !=
          Tetrodotoxin::Language::Visibility::Public ||
      !compatible_type(requirement.get_type(), selected->get_type()));
  return *selected;
}

auto Language::Interfaces::Structure::negotiate(
    const Abstract& requirement,
    const Abstract& candidate) const -> Relation {
  auto required = requirement.resolve().select<Language::Types::Structure>();
  auto supplied = candidate.resolve().select<Language::Types::Object>();
  if (!required || !supplied || !required->is_linked() ||
      !supplied->is_linked()) {
    return Relation::Rejected;
  }

  for (const Reference<Abstract>& retained :
       required->get_addressables(Tetrodotoxin::Language::Visibility::Public)) {
    auto field = retained.get().select<Language::Field>();
    if (!field || field->get_writability() != Language::Writability::Internal ||
        !select_field(*field, *supplied)) {
      return Relation::Rejected;
    }
  }

  return Relation::Satisfied;
}
