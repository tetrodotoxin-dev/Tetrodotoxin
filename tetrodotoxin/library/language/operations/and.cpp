// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/and.hpp"

#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;

static auto select_result_type(
    const Language::Model::Pack& left,
    const Language::Model::Pack& right) -> const Abstract& {
  const Abstract& selected_left = left.get_type().resolve();
  const Abstract& selected_right = right.get_type().resolve();
  if (&selected_left != &selected_right ||
      !selected_left.is<Language::Model::Types::Flag>()) {
    return Unknown::get_unknown();
  }

  return selected_left;
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

TTX_BINARY_OP(And);

auto Language::Operations::And::select_type(const Tetrodotoxin::Source::Abstract&) const
    -> Core::Option<const Language::Model::Type&> {
  auto inputs = get_inputs();
  const Model::Pack& left = inputs.get_data()[0].get();
  const Model::Pack& right = inputs.get_data()[1].get();
  return select_result_type(left, right).select<Language::Model::Type>();
}

auto Language::Operations::And::reaches_next_input(
    Count folded_input,
    const Constant& folded) const -> Bool {
  if (folded_input != 0) {
    return True;
  }

  auto type = folded.get_type().resolve().select<Model::Types::Flag>();
  auto validity = type ? type->get_validity(folded) : Core::Option<Bool>();
  return !validity || *validity;
}

auto Language::Operations::And::evaluate_constants(
    Memory::Allocator::Arena& domain)
    -> Utility::Result<
        Core::Option<Tetrodotoxin::Library::Language::Constant&>,
        Expression::Error> {
  auto inputs = get_inputs();
  Model::Pack& authored_left = inputs.get_data()[0].get();
  Model::Pack& authored_right = inputs.get_data()[1].get();
  auto left = get_folded_input(0);
  if (!left) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // False closes conjunction before the right edge matters. True reaches the
  // right input and keeps any failure attached to that authored Expression.
  auto result_type =
      get_type().select<Tetrodotoxin::Library::Language::Model::Types::Flag>();
  auto left_validity =
      result_type ? result_type->get_validity(*left) : Core::Option<Bool>();
  if (!left_validity || !result_type) {
    return Expression::Error::from_pack(
        Expression::Error::Type::InvalidConstant, authored_left);
  }

  if (!*left_validity) {
    return make_result(domain, *result_type, False);
  }

  auto right = get_folded_input(1);
  if (!right) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  auto right_validity = result_type->get_validity(*right);
  if (!right_validity) {
    return Expression::Error::from_pack(
        Expression::Error::Type::InvalidConstant, authored_right);
  }

  return make_result(domain, *result_type, *right_validity);
}
