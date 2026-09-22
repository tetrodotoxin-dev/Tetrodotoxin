// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/negate.hpp"

#include "perimortem/core/math.hpp"

#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;

static auto is_negatable_type(const Abstract& selected) -> Bool {
  return selected.visit<Tetrodotoxin::Library::Language::Model::Types::Signed>(
      [](const Tetrodotoxin::Library::Language::Model::Types::Signed& type) {
        return type.get_size() > 0 && type.get_size() <= sizeof(S64) ? True
                                                                     : False;
      },
      [](const Abstract& selected) {
        return selected
            .visit<Tetrodotoxin::Library::Language::Model::Types::Real>(
                [](const Tetrodotoxin::Library::Language::Model::Types::Real&
                       type) {
                  return type.get_size() == sizeof(R32) ||
                                 type.get_size() == sizeof(R64)
                             ? True
                             : False;
                },
                [](const Abstract&) { return False; });
      });
}

static auto select_result_type(const Language::Model::Pack& operand)
    -> const Abstract& {
  const Abstract& selected = operand.get_type().resolve();
  if (!selected.is<Language::Model::Type>() || !is_negatable_type(selected)) {
    return Unknown::get_unknown();
  }

  return selected;
}

static auto signed_inverse(
    const Tetrodotoxin::Library::Language::Model::Types::Signed& type,
    S64 operand,
    S64& result) -> Bool {
  if (__builtin_sub_overflow(S64(0), operand, &result)) {
    return False;
  }

  return Core::Math::is_representable(result, type.get_size());
}

TTX_UNARY_OP(Negate);

auto Language::Operations::Negate::select_type(const Tetrodotoxin::Source::Abstract&)
    const -> Core::Option<const Language::Model::Type&> {
  const Model::Pack& operand = get_inputs().get_data()[0].get();
  return select_result_type(operand).select<Language::Model::Type>();
}

auto Language::Operations::Negate::evaluate_constants(
    Memory::Allocator::Arena& domain)
    -> Utility::Result<
        Core::Option<Tetrodotoxin::Library::Language::Constant&>,
        Expression::Error> {
  const Abstract& selected = get_type().resolve();
  Model::Pack& authored_operand = get_inputs().get_data()[0].get();
  auto operand = get_folded_input(0);
  if (!operand) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // Linking fixes the exact result Type before folding. The visitors prove
  // only the Tetrodotoxin::Library::Language::Constant payload needed to
  // calculate its inverse.
  if (selected.is<Tetrodotoxin::Library::Language::Model::Types::Signed>()) {
    auto value = operand->select<Constants::Signed>();
    if (!value) {
      return Expression::Error::from_pack(
          Expression::Error::Type::InvalidConstant, authored_operand);
    }

    return selected.visit<
        Tetrodotoxin::Library::Language::Model::Types::Signed>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Signed& type)
            -> Utility::Result<
                Core::Option<Tetrodotoxin::Library::Language::Constant&>,
                Expression::Error> {
          S64 inverse = 0;
          if (!signed_inverse(type, value->get_value(), inverse)) {
            return Expression::Error(
                Expression::Error::Type::ArithmeticOverflow, *this);
          }

          return Constants::Signed::create_synthetic(domain, type, inverse);
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
    auto value = operand->select<Constants::Real>();
    if (!value) {
      return Expression::Error::from_pack(
          Expression::Error::Type::InvalidConstant, authored_operand);
    }

    return selected.visit<Tetrodotoxin::Library::Language::Model::Types::Real>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Real& type)
            -> Utility::Result<
                Core::Option<Tetrodotoxin::Library::Language::Constant&>,
                Expression::Error> {
          if (type.get_size() == sizeof(R32)) {
            R32 inverse = -R32(value->get_value());
            return Constants::Real::create_synthetic(
                domain, type, R64(inverse));
          }

          if (type.get_size() == sizeof(R64)) {
            return Constants::Real::create_synthetic(
                domain, type, -value->get_value());
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
