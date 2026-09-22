// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/generics/result.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/types/result.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Generics::Result::create(
    Perimortem::Core::View::Vector<Argument> arguments) const
    -> Perimortem::Core::Option<const Model::Type&> {
  if (arguments.get_size() != 2) {
    return {};
  }

  const Model::Type* value = arguments[0].find<const Model::Type&>();
  const Model::Type* error = arguments[1].find<const Model::Type&>();
  if (!value || !error || &value->resolve() == &error->resolve()) {
    return {};
  }

  auto flag = get_context()
                  .resolve_concept("Bool"_view)
                  .resolve()
                  .select<Model::Types::Flag>();
  if (!flag) {
    return {};
  }

  auto& arena = get_domain();
  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  name.concat("["_view);
  name.concat(value->get_name());
  name.concat(", "_view);
  name.concat(error->get_name());
  name.concat("]"_view);
  return arena.construct<Types::Result>(name.get_view(), *value, *error, *flag);
}
