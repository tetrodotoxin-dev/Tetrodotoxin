// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/less_equal.hpp"

#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
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

static auto select_operand_type(
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

  // Runtime values and Constants use the same exact operand Type. Conversion
  // belongs to a receiving typed owner before LessEqual construction.
  return left_resolved;
}

static auto make_result(
    Memory::Allocator::Arena& domain,
    const Tetrodotoxin::Library::Language::Model::Types::Flag& type,
    Bool value) -> Language::Constant& {
  if (value) {
    return Language::Constants::True::create_synthetic(domain, type);
  }

  return Language::Constants::False::create_synthetic(domain, type);
}

TTX_BINARY_OP(LessEqual);

auto Language::Operations::LessEqual::select_type(
    const Tetrodotoxin::Source::Abstract& context) const
    -> Core::Option<const Language::Model::Type&> {
  auto inputs = get_inputs();
  const Model::Pack& left = inputs.get_data()[0].get();
  const Model::Pack& right = inputs.get_data()[1].get();
  if (!select_operand_type(left, right).resolve().is<Language::Model::Type>()) {
    return {};
  }

  return context.resolve_concept("Bool"_view).select<Language::Model::Type>();
}

auto Language::Operations::LessEqual::evaluate_constants(
    Memory::Allocator::Arena& domain)
    -> Utility::Result<
        Core::Option<Tetrodotoxin::Library::Language::Constant&>,
        Expression::Error> {
  auto inputs = get_inputs();
  Model::Pack& authored_left = inputs.get_data()[0].get();
  Model::Pack& authored_right = inputs.get_data()[1].get();
  auto left = get_folded_input(0);
  auto right = get_folded_input(1);
  auto result_type =
      get_type().select<Tetrodotoxin::Library::Language::Model::Types::Flag>();
  if (!left || !right || !result_type) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  const Abstract& selected = left->get_type().resolve();

  // The operand domain was established before folding. These visitors prove
  // matching Tetrodotoxin::Library::Language::Constant payloads while every
  // result uses canonical Bool.
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

    return make_result(
        domain, *result_type,
        left_value->get_value() <= right_value->get_value());
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

    return make_result(
        domain, *result_type,
        left_value->get_value() <= right_value->get_value());
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
            return make_result(
                domain, *result_type,
                R32(left_value->get_value()) <= R32(right_value->get_value()));
          }

          if (type.get_size() == sizeof(R64)) {
            return make_result(
                domain, *result_type,
                left_value->get_value() <= right_value->get_value());
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
