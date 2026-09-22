// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/object/view.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Object::View::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> View& {
  auto& self = Tetrodotoxin::Source::Layouts::Addressable::create_synthetic(
      domain, "self"_view, receiver);
  return domain.construct_from<View>(
      [&]() -> View { return View(self, result); });
}
