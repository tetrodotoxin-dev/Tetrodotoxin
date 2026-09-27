// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/expressions/conversion.hpp"

#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto source_value(const Language::Model::Pack& source)
    -> Core::Option<const Language::Model::Types::Value&> {
  BAIL_IF(source.get_layout().get_size() != 1);
  return source.get_value_type(0)
      .resolve()
      .select<Language::Model::Types::Value>();
}

static auto is_numeric(const Language::Model::Types::Value& value) -> Bool {
  return Bool(
      value.is<Language::Model::Types::Unsigned>() ||
      value.is<Language::Model::Types::Signed>() ||
      value.is<Language::Model::Types::Real>());
}

static constexpr auto unsigned_max(Count size) -> U64 {
  return size >= sizeof(U64) ? U64(-1) : (U64(1) << (size * 8)) - 1;
}

static constexpr auto signed_min(Count size) -> S64 {
  return size >= sizeof(S64) ? S64(U64(1) << 63) : -(S64(1) << (size * 8 - 1));
}

static constexpr auto signed_max(Count size) -> S64 {
  return size >= sizeof(S64) ? S64(U64(-1) >> 1)
                             : (S64(1) << (size * 8 - 1)) - 1;
}

auto Language::Expressions::Conversion::create(
    Memory::Allocator::Arena& arena,
    const Model::Types::Value& target,
    Model::Pack& source) -> Conversion& {
  return arena.construct_from<Conversion>(
      [&]() -> Conversion { return Conversion(arena, target, source); });
}

auto Language::Expressions::Conversion::accepts(
    const Model::Types::Value& target,
    const Model::Pack& source) -> Bool {
  auto selected = source_value(source);
  return selected && !source.get_layout().get_name(0) && is_numeric(target) &&
         is_numeric(*selected);
}

auto Language::Expressions::Conversion::link(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Tetrodotoxin::Source::Abstract&,
    Core::Option<const Tetrodotoxin::Source::Abstract&>) -> Bool {
  if (accepts(target.get(), source.get())) {
    return True;
  }
  cursor.create_expression_error(
      get_anchor(),
      "Scalar construction requires one numeric source value."_view,
      "Use Signed, Unsigned, or Real values on both sides of the conversion."_view);
  return False;
}

auto Language::Expressions::Conversion::link_restored(
    const Tetrodotoxin::Source::Abstract&,
    Core::Option<const Tetrodotoxin::Source::Abstract&>) -> Bool {
  return accepts(target.get(), source.get());
}

auto Language::Expressions::Conversion::finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor)
    -> void {
  source.get().finalize(cursor);
  Expression::finalize(cursor);
}

static auto folded_constant(Language::Model::Pack& source)
    -> Core::Option<const Language::Constant&> {
  auto folded = Language::Expression::fold(source);
  Core::Option<Language::Model::Pack&> value;
  folded.visit(
      [&](const Core::Option<Language::Model::Pack&>& selected) {
        value = selected;
      },
      [](const Language::Expression::Error&) {});
  BAIL_IF(!value);
  auto selected = value->get_layout().get_abstract(0);
  return selected ? selected->select<Language::Constant>()
                  : Core::Option<const Language::Constant&>();
}

auto Language::Expressions::Conversion::evaluate()
    -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
  auto constant = folded_constant(source.get());
  if (!constant) {
    return Core::Option<Model::Pack&>();
  }

  auto target_unsigned = target.get().select<Model::Types::Unsigned>();
  auto target_signed = target.get().select<Model::Types::Signed>();
  auto target_real = target.get().select<Model::Types::Real>();
  auto source_unsigned = constant->select<Constants::Unsigned>();
  auto source_signed = constant->select<Constants::Signed>();
  auto source_real = constant->select<Constants::Real>();

  if (target_unsigned) {
    U64 maximum = unsigned_max(target_unsigned->get_size());
    U64 value = 0;
    if (source_unsigned) {
      value = source_unsigned->get_value() > maximum
                  ? maximum
                  : source_unsigned->get_value();
    } else if (source_signed) {
      value = source_signed->get_value() <= 0 ? 0
              : U64(source_signed->get_value()) > maximum
                  ? maximum
                  : U64(source_signed->get_value());
    } else if (source_real) {
      R64 selected = source_real->get_value();
      if (!__builtin_isnan(selected) && selected > 0.0) {
        value = selected >= R64(maximum) ? maximum : U64(selected);
      }
    } else {
      return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
    }
    return Constants::Unsigned::create_synthetic(
        arena, *target_unsigned, value);
  }

  if (target_signed) {
    S64 minimum = signed_min(target_signed->get_size());
    S64 maximum = signed_max(target_signed->get_size());
    S64 value = 0;
    if (source_unsigned) {
      value = source_unsigned->get_value() > U64(maximum)
                  ? maximum
                  : S64(source_unsigned->get_value());
    } else if (source_signed) {
      value = source_signed->get_value() < minimum ? minimum
              : source_signed->get_value() > maximum
                  ? maximum
                  : source_signed->get_value();
    } else if (source_real) {
      R64 selected = source_real->get_value();
      if (!__builtin_isnan(selected)) {
        value = selected <= R64(minimum)   ? minimum
                : selected >= R64(maximum) ? maximum
                                           : S64(selected);
      }
    } else {
      return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
    }
    return Constants::Signed::create_synthetic(arena, *target_signed, value);
  }

  if (target_real) {
    R64 value = source_unsigned ? R64(source_unsigned->get_value())
                : source_signed ? R64(source_signed->get_value())
                : source_real   ? source_real->get_value()
                                : 0.0;
    if (!source_unsigned && !source_signed && !source_real) {
      return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
    }
    if (target_real->get_size() == sizeof(R32)) {
      value = R64(R32(value));
    }
    return Constants::Real::create_synthetic(arena, *target_real, value);
  }

  return Expression::Error(
      Expression::Error::Type::InvalidOperationType, *this);
}
