// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/generics/access.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/types/access.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Generics::Access::create(
    Perimortem::Core::View::Vector<Argument> arguments) const
    -> Perimortem::Core::Option<const Language::Model::Type&> {
  auto& arena = get_domain();
  if (arguments.get_size() != 1) {
    return {};
  }

  const Language::Model::Type* element =
      arguments.get_data()[0].find<const Language::Model::Type&>();
  if (!element) {
    return {};
  }
  if (element->get_layout().is_empty()) {
    return {};
  }

  auto size_type = get_context()
                       .resolve_concept("U64"_view)
                       .resolve()
                       .select<Language::Model::Type>();
  auto flag_type = get_context()
                       .resolve_concept("Bool"_view)
                       .resolve()
                       .select<Language::Model::Type>();
  if (!size_type || !flag_type) {
    return {};
  }

  auto view = get_context()
                  .resolve_concept("View"_view)
                  .resolve()
                  .select<Language::Generic>();
  if (!view) {
    return {};
  }
  const Perimortem::Core::Static::Vector<Argument, 1> view_arguments = {
    {Argument(*element)}};
  auto view_type =
      view->materialize(view_arguments.get_view())
          .visit(
              [](const Language::Model::Type& selected)
                  -> Perimortem::Core::Option<const Language::Model::Type&> {
                return selected;
              },
              [](const Language::Generic::Failure&)
                  -> Perimortem::Core::Option<const Language::Model::Type&> {
                return {};
              });
  if (!view_type) {
    return {};
  }

  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  name.concat("["_view);
  name.concat(element->get_name());
  name.concat("]"_view);
  return arena.construct<Types::Access>(
      arena, name.get_view(), *element, *size_type, *flag_type, *view_type);
}
