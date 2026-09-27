// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/types/value.hpp"

#include "tetrodotoxin/library/language/expressions/conversion.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library::Language;

auto Model::Types::Value::accepts(const Model::Pack& source) const -> Bool {
  auto selected = source.get_type().resolve().select<Value>();
  return selected && is_equivalent(*selected);
}

auto Model::Types::Value::is_equivalent(const Value& selected) const -> Bool {
  BAIL_IF(selected.get_width() != get_width());
  return Bool(
      (is<Flag>() && selected.is<Flag>()) ||
      (is<Real>() && selected.is<Real>()) ||
      (is<Signed>() && selected.is<Signed>()) ||
      (is<Unsigned>() && selected.is<Unsigned>()));
}

auto Model::Types::Value::create_supplied(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    Model::Pack& source,
    Option<const Abstract&>,
    Option<Tetrodotoxin::Source::Lexical::Anchor> anchor) const -> Option<Model::Pack&> {
  if (accepts(source)) {
    return source;
  }
  if (!Expressions::Conversion::accepts(*this, source)) {
    cursor.create_expression_error(
        anchor, "Selected scalar Type cannot construct this value."_view,
        "Supply one Signed, Unsigned, or Real value for explicit conversion."_view);
    return {};
  }
  return Expressions::Conversion::create(cursor.get_arena(), *this, source);
}

auto Model::Types::Value::create_supplied_restored(
    Perimortem::Memory::Allocator::Arena& arena,
    Model::Pack& source,
    Option<const Abstract&>) const -> Option<Model::Pack&> {
  if (accepts(source)) {
    return source;
  }
  BAIL_IF(!Expressions::Conversion::accepts(*this, source));
  return Expressions::Conversion::create(arena, *this, source);
}
