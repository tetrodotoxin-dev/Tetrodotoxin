// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/enum/name.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Enum::Name::create(
    Memory::Allocator::Arena& domain,
    const Language::Types::Enumeration& enumeration,
    const Language::Model::Type& result) -> Name& {
  Tetrodotoxin::Source::Layouts::Addressable& self =
      Tetrodotoxin::Source::Layouts::Addressable::create_synthetic(
          domain, "self"_view, enumeration);
  return domain.construct_from<Name>(
      [&]() -> Name { return Name(self, enumeration, result); });
}

auto Builtin::Enum::Name::fold_call(
    Memory::Allocator::Arena& domain,
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  BAIL_IF(!receiver || !arguments.get_layout().is_empty());

  auto constant = receiver->select_identity<Language::Constants::Enumeration>();
  BAIL_IF(!constant || &constant->get_type() != &enumeration);
  Core::View::Bytes name = enumeration.find_case_name(constant->get_value());
  return Language::Constants::Bytes::create_synthetic(
      domain, result_type, name);
}
