// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/divide.hpp"

#include "perimortem/core/math.hpp"

#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;

static auto select_result_type(
    const Language::Model::Pack& left,
    const Language::Model::Pack& right) -> const Abstract& {
  const Abstract& left_resolved = left.get_type().resolve();
  const Abstract& right_resolved = right.get_type().resolve();
  auto left_value = left_resolved.select<Language::Model::Types::Value>();
  auto right_value = right_resolved.select<Language::Model::Types::Value>();
  if (!left_value || !right_value || !left_value->is_equivalent(*right_value) ||
      (!left_resolved.is<Language::Model::Types::Unsigned>() &&
       !left_resolved.is<Language::Model::Types::Signed>() &&
       !left_resolved.is<Language::Model::Types::Real>())) {
    return Unknown::get_unknown();
  }

  // Divide keeps both authored Types exact. A receiving typed owner performs
  // any conversion before construction so runtime and
  // Tetrodotoxin::Library::Language::Constant inputs agree.
  return left_resolved;
}

TTX_BINARY_OP(Divide);

auto Language::Operations::Divide::select_type(const Tetrodotoxin::Source::Abstract&)
    const -> Core::Option<const Language::Model::Type&> {
  auto inputs = get_inputs();
  const Model::Pack& left = inputs.get_data()[0].get();
  const Model::Pack& right = inputs.get_data()[1].get();
  return select_result_type(left, right).select<Language::Model::Type>();
}

auto Language::Operations::Divide::evaluate_constants(
    Memory::Allocator::Arena& domain)
    -> Utility::Result<
        Core::Option<Tetrodotoxin::Library::Language::Constant&>,
        Expression::Error> {
  const Abstract& selected = get_type().resolve();
  auto inputs = get_inputs();
  Model::Pack& authored_left = inputs.get_data()[0].get();
  Model::Pack& authored_right = inputs.get_data()[1].get();
  auto left = get_folded_input(0);
  auto right = get_folded_input(1);
  if (!left || !right) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // Type selection fixes the payload domain before folding. Integer guards
  // run before host arithmetic so zero and the signed endpoint stay failures.
  if (selected.is<Tetrodotoxin::Library::Language::Model::Types::Signed>()) {
    auto left_value = left->select<Constants::Signed>();
    auto right_value = right->select<Constants::Signed>();
    if (!left_value) {
      return Expression::Error::from_pack(
          Expression::Error::Type::InvalidConstant, authored_left);
    }

    if (!right_value) {
      return Expression::Error::from_pack(
          Expression::Error::Type::InvalidConstant, authored_right);
    }

    return selected.visit<
        Tetrodotoxin::Library::Language::Model::Types::Signed>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Signed& type)
            -> Utility::Result<
                Core::Option<Tetrodotoxin::Library::Language::Constant&>,
                Expression::Error> {
          S64 divisor = right_value->get_value();
          if (divisor == 0) {
            return Expression::Error(
                Expression::Error::Type::DivisionByZero, *this);
          }

          S64 value = 0;
          if (divisor == -1) {
            Bool overflow =
                __builtin_sub_overflow(S64(0), left_value->get_value(), &value);
            if (overflow ||
                !Core::Math::is_representable(value, type.get_size())) {
              return Expression::Error(
                  Expression::Error::Type::ArithmeticOverflow, *this);
            }
          } else {
            value = left_value->get_value() / divisor;
          }

          if (!Core::Math::is_representable(value, type.get_size())) {
            return Expression::Error(
                Expression::Error::Type::ArithmeticOverflow, *this);
          }

          return Constants::Signed::create_synthetic(domain, type, value);
        },
        [&](const Abstract&)
            -> Utility::Result<
                Core::Option<Tetrodotoxin::Library::Language::Constant&>,
                Expression::Error> {
          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        });
  }

  if (selected.is<Tetrodotoxin::Library::Language::Model::Types::Unsigned>()) {
    auto left_value = left->select<Constants::Unsigned>();
    auto right_value = right->select<Constants::Unsigned>();
    if (!left_value) {
      return Expression::Error::from_pack(
          Expression::Error::Type::InvalidConstant, authored_left);
    }

    if (!right_value) {
      return Expression::Error::from_pack(
          Expression::Error::Type::InvalidConstant, authored_right);
    }

    return selected.visit<
        Tetrodotoxin::Library::Language::Model::Types::Unsigned>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Unsigned& type)
            -> Utility::Result<
                Core::Option<Tetrodotoxin::Library::Language::Constant&>,
                Expression::Error> {
          U64 divisor = right_value->get_value();
          if (divisor == 0) {
            return Expression::Error(
                Expression::Error::Type::DivisionByZero, *this);
          }

          U64 value = left_value->get_value() / divisor;
          if (!Core::Math::is_representable(value, type.get_size())) {
            return Expression::Error(
                Expression::Error::Type::ArithmeticOverflow, *this);
          }

          return Constants::Unsigned::create_synthetic(domain, type, value);
        },
        [&](const Abstract&)
            -> Utility::Result<
                Core::Option<Tetrodotoxin::Library::Language::Constant&>,
                Expression::Error> {
          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        });
  }

  if (selected.is<Tetrodotoxin::Library::Language::Model::Types::Real>()) {
    auto left_value = left->select<Constants::Real>();
    auto right_value = right->select<Constants::Real>();
    if (!left_value) {
      return Expression::Error::from_pack(
          Expression::Error::Type::InvalidConstant, authored_left);
    }

    if (!right_value) {
      return Expression::Error::from_pack(
          Expression::Error::Type::InvalidConstant, authored_right);
    }

    return selected.visit<Tetrodotoxin::Library::Language::Model::Types::Real>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Real& type)
            -> Utility::Result<
                Core::Option<Tetrodotoxin::Library::Language::Constant&>,
                Expression::Error> {
          if (type.get_size() == sizeof(R32)) {
            R32 value =
                R32(left_value->get_value()) / R32(right_value->get_value());
            return Constants::Real::create_synthetic(domain, type, R64(value));
          }

          if (type.get_size() == sizeof(R64)) {
            return Constants::Real::create_synthetic(
                domain, type,
                left_value->get_value() / right_value->get_value());
          }

          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        },
        [&](const Abstract&)
            -> Utility::Result<
                Core::Option<Tetrodotoxin::Library::Language::Constant&>,
                Expression::Error> {
          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        });
  }

  return Expression::Error(
      Expression::Error::Type::InvalidOperationType, *this);
}
