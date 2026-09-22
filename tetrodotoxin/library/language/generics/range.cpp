// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/generics/range.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Generics::Range::create(Perimortem::Core::View::Vector<Argument> arguments)
    const -> Perimortem::Core::Option<const Language::Model::Type&> {
  auto& arena = get_domain();
  if (arguments.get_size() != 1) {
    return {};
  }

  const Language::Model::Type* element =
      arguments.get_data()[0].find<const Language::Model::Type&>();
  if (element == nullptr ||
      (!element->is<Tetrodotoxin::Library::Language::Model::Types::Signed>() &&
       !element
            ->is<Tetrodotoxin::Library::Language::Model::Types::Unsigned>())) {
    return {};
  }

  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  name.concat("["_view);
  name.concat(element->get_name());
  name.concat("]"_view);
  return arena.construct<Types::Range>(name.get_view(), *element);
}
