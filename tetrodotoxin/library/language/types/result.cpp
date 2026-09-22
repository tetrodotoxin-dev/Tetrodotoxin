// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/result.hpp"

#include "tetrodotoxin/library/language/constants/result.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library::Language;

auto Types::Result::create_default(Memory::Allocator::Arena& arena) const
    -> Core::Option<Model::Pack&> {
  auto selected = value.create_default(arena);
  BAIL_IF(!selected);
  auto created = Constants::Result::create_value(arena, *this, *selected);
  return created ? Core::Option<Model::Pack&>(*created)
                 : Core::Option<Model::Pack&>();
}

auto Types::Result::fold_propagation(Model::Pack& source) const
    -> Utility::Result<Core::Option<Model::Pack&>, Bool> {
  auto selected = Constants::Result::select(source);
  if (!selected || &selected->get_type() != this) {
    return False;
  }

  return selected->get_kind() == Kind::Value
             ? Core::Option<Model::Pack&>(
                   const_cast<Model::Pack&>(selected->get_payload()))
             : Core::Option<Model::Pack&>();
}

auto Types::Result::accepts(const Model::Pack& source) const -> Bool {
  if (source.fits(*this)) {
    return True;
  }

  Bool accepts_value = source.fits_into(value);
  Bool accepts_error = source.fits_into(error);
  return accepts_value != accepts_error;
}

auto Types::Result::create_fitted(
    Memory::Allocator::Arena& arena,
    Model::Pack& source) const -> Core::Option<Model::Pack&> {
  auto fitted = Constants::Result::create_fitted(arena, *this, source);
  return fitted ? Core::Option<Model::Pack&>(*fitted)
                : Core::Option<Model::Pack&>();
}

auto Types::Result::validate_layout(Tetrodotoxin::Source::Lexical::Cursor& cursor) const
    -> Bool {
  if (!value.get_layout().is_empty() && !error.get_layout().is_empty()) {
    return True;
  }

  cursor.create_error(
      "Result alternatives must each produce one nonempty value Type."_view,
      "Replace the empty value or error Type before using this Result."_view);
  return False;
}
