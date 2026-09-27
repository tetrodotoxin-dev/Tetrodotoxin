// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/bytes.hpp"

#include "tetrodotoxin/source/none.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library::Language;

auto Constants::Bytes::resolve_concept(Core::View::Bytes name) const
    -> const Abstract& {
  if (name != "resource"_view) {
    return None::get_none();
  }
  return resource.visit(
      []() -> const Abstract& { return None::get_none(); },
      [](const Reference<const Tetrodotoxin::Language::Resource>& selected)
          -> const Abstract& { return selected.get(); });
}

auto Constants::Bytes::visit_concepts(
    Tetrodotoxin::Source::Abstract::Visitor visitor) const -> void {
  auto selected = get_resource();
  if (selected) {
    visitor("resource"_view, *selected);
  }
}
