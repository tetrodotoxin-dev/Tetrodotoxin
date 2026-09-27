// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/fixed/view.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Fixed::View::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> View& {
  Tetrodotoxin::Source::Layouts::Addressable& self =
      Tetrodotoxin::Source::Layouts::Addressable::create_synthetic(
          domain, "self"_view, receiver);
  return domain.construct_from<View>(
      [&]() -> View { return View(self, result); });
}

auto Builtin::Fixed::View::fold_call(
    Memory::Allocator::Arena& domain,
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  BAIL_IF(!receiver || !arguments.get_layout().is_empty());

  auto bytes = receiver->select_identity<Language::Constants::Bytes>();
  BAIL_IF(!bytes);
  return Language::Constants::Bytes::create_synthetic(
      domain, result_type, bytes->get_value(), bytes->get_resource());
}
