// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/generics/option.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/types/option.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Generics::Option::create(
    Perimortem::Core::View::Vector<Argument> arguments) const
    -> Perimortem::Core::Option<const Language::Model::Type&> {
  auto& arena = get_domain();
  if (arguments.get_size() != 1) {
    return {};
  }

  const Language::Model::Type* element =
      arguments.get_data()[0].find<const Language::Model::Type&>();
  if (element == nullptr) {
    return {};
  }
  if (element->get_layout().is_empty()) {
    return {};
  }

  auto flag = get_context()
                  .resolve_concept("Bool"_view)
                  .resolve()
                  .select<Model::Types::Flag>();
  if (!flag) {
    return {};
  }

  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  name.concat("["_view);
  name.concat(element->get_name());
  name.concat("]"_view);
  return arena.construct<Types::Option>(name.get_view(), *element, *flag);
}
