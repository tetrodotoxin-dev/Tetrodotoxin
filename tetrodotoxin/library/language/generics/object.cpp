// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/generics/object.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/types/object_storage.hpp"

using namespace Tetrodotoxin::Library::Language;

static auto materialize_contiguous(
    const Tetrodotoxin::Source::Abstract& context,
    Perimortem::Core::View::Bytes name,
    const Model::Type& element)
    -> Perimortem::Core::Option<const Model::Type&> {
  auto generic = context.resolve_concept(name).resolve().select<Generic>();
  if (!generic) {
    return {};
  }

  const Perimortem::Core::Static::Vector<Generic::Argument, 1> arguments = {
    {Generic::Argument(element)}};
  return generic->materialize(arguments.get_view())
      .visit(
          [](const Model::Type& type)
              -> Perimortem::Core::Option<const Model::Type&> { return type; },
          [](const Generic::Failure&)
              -> Perimortem::Core::Option<const Model::Type&> { return {}; });
}

auto Generics::Object::create(
    Perimortem::Core::View::Vector<Argument> arguments) const
    -> Perimortem::Core::Option<const Language::Model::Type&> {
  auto& arena = get_domain();
  if (arguments.get_size() != 1) {
    return {};
  }

  const Model::Type* element =
      arguments.get_data()[0].find<const Model::Type&>();
  if (!element || element->get_layout().is_empty()) {
    return {};
  }

  auto size_type =
      get_context().resolve_concept("U64"_view).resolve().select<Model::Type>();
  auto flag_type = get_context()
                       .resolve_concept("Bool"_view)
                       .resolve()
                       .select<Model::Type>();
  auto view = materialize_contiguous(get_context(), "View"_view, *element);
  auto access = materialize_contiguous(get_context(), "Access"_view, *element);
  if (!size_type || !flag_type || !view || !access) {
    return {};
  }

  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  name.concat("["_view);
  name.concat(element->get_name());
  name.concat("]"_view);
  return arena.construct<Types::ObjectStorage>(
      arena, name.get_view(), *element, *size_type, *flag_type, *view, *access);
}
