// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/generics/fixed.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/library/language/types/fixed.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Generics::Fixed::create(Perimortem::Core::View::Vector<Argument> arguments)
    const -> Perimortem::Core::Option<const Language::Model::Type&> {
  auto& arena = get_domain();
  if (arguments.get_size() != 2) {
    return {};
  }

  const auto* argument_data = arguments.get_data();
  const Language::Model::Type* element =
      argument_data[0].find<const Language::Model::Type&>();
  const ::U64* extent = argument_data[1].find<::U64>();
  if (!element || !extent || *extent == 0) {
    return {};
  }
  if (element->get_layout().is_empty()) {
    return {};
  }

  auto access = get_context()
                    .resolve_concept("Access"_view)
                    .resolve()
                    .select<Language::Generic>();
  auto view = get_context()
                  .resolve_concept("View"_view)
                  .resolve()
                  .select<Language::Generic>();
  if (!access || !view) {
    return {};
  }
  const Perimortem::Core::Static::Vector<Argument, 1> access_arguments = {
    {Argument(*element)}};
  auto access_type =
      access->materialize(access_arguments.get_view())
          .visit(
              [](const Language::Model::Type& selected)
                  -> Perimortem::Core::Option<const Language::Model::Type&> {
                return selected;
              },
              [](const Language::Generic::Failure&)
                  -> Perimortem::Core::Option<const Language::Model::Type&> {
                return {};
              });
  if (!access_type) {
    return {};
  }
  auto view_type =
      view->materialize(access_arguments.get_view())
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
  Perimortem::Serialization::Stream::Textual<Perimortem::Memory::Managed::Bytes>
      output(name);
  output << "["_view << element->get_name() << ","_view << *extent << "]"_view;
  return arena.construct<Types::Fixed>(
      arena, name.get_view(), *element, *extent, *access_type, *view_type);
}
