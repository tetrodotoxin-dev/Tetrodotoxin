// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/graphics/compiler.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

static auto find_configured_type(
    Core::View::Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>
        configured,
    const Tetrodotoxin::Source::Type& candidate) -> Core::Option<Count> {
  Core::Option<Count> selected;
  for (Count index = 0; index < configured.get_size(); index++) {
    if (&configured.get_data()[index].get().resolve() == &candidate.resolve()) {
      BAIL_IF(selected);
      selected = index;
    }
  }
  return selected;
}

static auto retain_hosted(
    Memory::Managed::Vector<Terminal::Graphics::Products::Hosted>& hosted,
    const Library::Language::Field& field,
    const Library::Language::Types::Object& object,
    const Tetrodotoxin::Source::Type& requirement,
    Core::View::Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>
        configured,
    Core::Option<Count> element_index = {}) -> Bool {
  if (!object.satisfies(requirement)) {
    return True;
  }

  auto type_index = find_configured_type(configured, object);
  BAIL_IF(!type_index);
  hosted.insert(
      Terminal::Graphics::Products::Hosted(
          field, *type_index, element_index));
  return True;
}

auto Terminal::Graphics::Compiler::compile(
    Memory::Allocator::Arena& arena,
    const Scene::Language::Monograph& scene,
    const Tetrodotoxin::Source::Type& requirement,
    Core::View::Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>
        configured) const -> Core::Option<Products> {
  BAIL_IF(!scene.is_finalized() || configured.is_empty());

  Memory::Managed::Vector<Products::Hosted> hosted(arena);
  for (const Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>& declaration :
       scene.get_instance().get_addressables()) {
    auto field = declaration.get().select<Library::Language::Field>();
    if (!field ||
        field->get_definition().get_visibility() !=
            Language::Visibility::Private ||
        field->get_writability() != Library::Language::Writability::Internal) {
      continue;
    }

    auto object = field->get_type().select<Library::Language::Types::Object>();
    if (object) {
      BAIL_IF(!retain_hosted(
          hosted, *field, *object, requirement, configured));
      continue;
    }

    auto fixed = field->get_type().select<Library::Language::Types::Fixed>();
    auto element =
        fixed ? fixed->get_element_type()
                    .select<Library::Language::Types::Object>()
              : Core::Option<const Library::Language::Types::Object&>();
    if (!fixed || !element || fixed->get_extent() > U64(Count(-1))) {
      continue;
    }

    for (Count index = 0; index < Count(fixed->get_extent()); index++) {
      BAIL_IF(!retain_hosted(
          hosted, *field, *element, requirement, configured, index));
    }
  }
  return Products(scene, hosted.get_view());
}
