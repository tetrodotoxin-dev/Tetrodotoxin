// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/bool.hpp"

#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Boolean::get_validity(const Model::Pack& value) const
    -> Option<Bool> {
  auto first = value.get_layout().get_abstract(0);
  BAIL_IF(!first);

  auto flag = first->select<Constants::Flag>();
  BAIL_IF(!flag || &flag->get_type().resolve() != &resolve());
  return flag->get_value();
}

auto Types::Boolean::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::False::create_synthetic(arena, *this);
}

auto Types::Boolean::fold_propagation(Model::Pack& source) const
    -> Perimortem::Utility::Result<Option<Model::Pack&>, Bool> {
  auto validity = get_validity(source);
  if (!validity) {
    return False;
  }

  return *validity ? Option<Model::Pack&>(source) : Option<Model::Pack&>();
}
