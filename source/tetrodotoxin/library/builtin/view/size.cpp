// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/view/size.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::View::Size::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> Size& {
  Tetrodotoxin::Source::Layouts::Addressable& self =
      Tetrodotoxin::Source::Layouts::Addressable::create_synthetic(
          domain, "self"_view, receiver);
  return domain.construct_from<Size>(
      [&]() -> Size { return Size(self, result); });
}
