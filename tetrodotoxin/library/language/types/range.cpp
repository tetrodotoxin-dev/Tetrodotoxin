// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/range.hpp"

#include "tetrodotoxin/library/language/constants/range.hpp"
#include "tetrodotoxin/source/addressable.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Range::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Range::create_synthetic(arena, *this);
}

auto Types::Range::accepts_iteration(const Tetrodotoxin::Source::Layout& bindings) const
    -> Bool {
  auto binding = bindings.get_abstract(0);
  auto addressable = binding ? binding->select<Tetrodotoxin::Source::Addressable>()
                             : Option<const Tetrodotoxin::Source::Addressable&>();
  return bindings.get_size() == 1 && addressable &&
         &addressable->get_type().resolve() == &element.resolve();
}
