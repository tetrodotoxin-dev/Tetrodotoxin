// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/view/is_empty.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::View::IsEmpty::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> IsEmpty& {
  Tetrodotoxin::Source::Layouts::Addressable& self =
      Tetrodotoxin::Source::Layouts::Addressable::create_synthetic(
          domain, "self"_view, receiver);
  return domain.construct_from<IsEmpty>(
      [&]() -> IsEmpty { return IsEmpty(self, result); });
}

auto Builtin::View::IsEmpty::fold_call(
    Memory::Allocator::Arena& domain,
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  auto value = receiver
                   ? receiver->select_identity<Language::Constants::Bytes>()
                   : Core::Option<const Language::Constants::Bytes&>();
  auto flag = result_type.resolve().select<Language::Model::Types::Flag>();
  BAIL_IF(!value || !flag || !arguments.get_layout().is_empty());
  return Language::Constants::Flag::create_synthetic(
      domain, *flag, value->get_value().is_empty());
}
