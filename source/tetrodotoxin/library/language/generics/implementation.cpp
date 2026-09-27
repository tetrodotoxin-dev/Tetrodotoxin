// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/generics/implementation.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/types/implementation.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Generics::Implementation::create(
    Perimortem::Core::View::Vector<Argument> arguments) const
    -> Perimortem::Core::Option<const Model::Type&> {
  BAIL_IF(arguments.get_size() != 1);
  const Generic::SemanticType* selected =
      arguments.get_data()[0].find<Generic::SemanticType>();
  const Tetrodotoxin::Source::Type* requirement = selected ? &selected->get() : nullptr;
  BAIL_IF(!requirement);

  auto& arena = get_domain();
  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  name.concat("["_view);
  name.concat(requirement->get_name());
  name.concat("]"_view);
  return arena.construct<Types::Implementation>(name.get_view(), *requirement);
}
