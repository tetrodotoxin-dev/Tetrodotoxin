// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/not.hpp"

#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;

static auto select_result_type(const Language::Model::Pack& operand)
    -> const Abstract& {
  const Abstract& selected = operand.get_type().resolve();
  if (!selected.is<Language::Model::Types::Flag>()) {
    return Unknown::get_unknown();
  }

  return selected;
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

TTX_UNARY_OP(Not);

auto Language::Operations::Not::select_type(const Tetrodotoxin::Source::Abstract&) const
    -> Core::Option<const Language::Model::Type&> {
  const Model::Pack& operand = get_inputs().get_data()[0].get();
  return select_result_type(operand).select<Language::Model::Type>();
}

auto Language::Operations::Not::evaluate_constants(
    Memory::Allocator::Arena& domain)
    -> Utility::Result<
        Core::Option<Tetrodotoxin::Library::Language::Constant&>,
        Expression::Error> {
  Model::Pack& authored_operand = get_inputs().get_data()[0].get();
  auto operand = get_folded_input(0);
  if (!operand) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // The selected Flag owns value interpretation. True and False remain the
  // canonical semantic results without exposing a storage Type here.
  auto result_type =
      get_type().select<Tetrodotoxin::Library::Language::Model::Types::Flag>();
  auto validity =
      result_type ? result_type->get_validity(*operand) : Core::Option<Bool>();
  if (!validity || !result_type) {
    return Expression::Error::from_pack(
        Expression::Error::Type::InvalidConstant, authored_operand);
  }

  return make_result(domain, *result_type, !*validity);
}
