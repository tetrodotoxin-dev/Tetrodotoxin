// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/modulo.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/interpreter/operation.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/r32.hpp"
#include "tetrodotoxin/library/language/types/s8.hpp"
#include "tetrodotoxin/library/language/types/u16.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness LibraryModulo = {
  .name = "Tetrodotoxin::Library::Language::Operations::Modulo"_view,
};

static auto link_operation(Operation& operation, const Abstract& context)
    -> Bool {
  Allocator::Arena transaction;
  Errors errors;
  Tokenizer tokenizer(transaction, {}, "<operation>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  return operation.link(cursor, context);
}

class ModuloExpression : public Expression {
 public:
  ModuloExpression(View::Bytes name, const Abstract& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }

 private:
  View::Bytes name;
  const Abstract& type;
};

class ModuloUnresolvedType : public Tetrodotoxin::Source::Type {
 public:
  auto get_name() const -> View::Bytes override { return "Unresolved"_view; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve() const -> const Abstract& override {
    return Unknown::get_unknown();
  }
  auto resolve_concept(View::Bytes) const -> const Abstract& override {
    return Unknown::get_unknown();
  }
};

class ModuloFoldInput : public Operation {
 public:
  ModuloFoldInput(
      Allocator::Arena& domain,
      Model::Pack& input,
      Tetrodotoxin::Library::Language::Constant& result,
      const Model::Type& type,
      Bool fails = False)
      : Operation(
            domain,
            Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 1>{{input}},
            {}),
        result(result),
        type(type),
        fails(fails) {}

  auto get_name() const -> View::Bytes override { return "Fold input"_view; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto get_evaluations() const -> Count { return evaluations; }

 protected:
  auto evaluate_constants(Allocator::Arena&) -> Result<
      Option<Tetrodotoxin::Library::Language::Constant&>,
      Expression::Error> override {
    evaluations++;
    if (fails) {
      return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
    }

    return result;
  }

  auto select_type(const Abstract&) const
      -> Option<const Model::Type&> override {
    return type;
  }

 private:
  Tetrodotoxin::Library::Language::Constant& result;
  const Model::Type& type;
  Bool fails;
  Count evaluations = 0;
};

static auto selected(
    const Result<Option<Model::Pack&>, Expression::Error>& result)
    -> Option<Tetrodotoxin::Library::Language::Constant&> {
  return result.visit(
      [](const Option<Model::Pack&>& folded)
          -> Option<Tetrodotoxin::Library::Language::Constant&> {
        return folded.visit(
            []() -> Option<Tetrodotoxin::Library::Language::Constant&> {
              return {};
            },
            [](Model::Pack& selected)
                -> Option<Tetrodotoxin::Library::Language::Constant&> {
              return selected
                  .select_identity<Tetrodotoxin::Library::Language::Constant>();
            });
      },
      [](const Expression::Error&)
          -> Option<Tetrodotoxin::Library::Language::Constant&> { return {}; });
}

static auto reports(
    const Result<Option<Model::Pack&>, Expression::Error>& result,
    Expression::Error::Type expected,
    const Abstract& origin) -> Bool {
  return result.visit(
      [](const Option<Model::Pack&>&) { return False; },
      [&](const Expression::Error& error) {
        return error.get_type() == expected && &error.get_subject() == &origin
                   ? True
                   : False;
      });
}

template <typename constant_type, typename value_type>
static auto get_value(
    const Tetrodotoxin::Library::Language::Constant& expression)
    -> Option<value_type> {
  return expression.visit<constant_type>(
      [](const constant_type& constant) -> Option<value_type> {
        return constant.get_value();
      },
      [](const Abstract&) -> Option<value_type> { return {}; });
}

template <typename constant_type, typename value_type>
static auto value_is(
    const Tetrodotoxin::Library::Language::Constant& expression,
    value_type expected) -> Bool {
  auto value = get_value<constant_type, value_type>(expression);
  return value && *value == expected ? True : False;
}

PERIMORTEM_UNIT_TEST(LibraryModulo, type_selection) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::S8 s8;
  Types::U8 u8;
  Types::U16 u16;
  Types::R32 r32;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[U8,1]"_view, resolve_library_unsigned(source, "U8"_view), 1);
  ModuloUnresolvedType unresolved_type;
  ModuloExpression signed_left("signed left"_view, s8);
  ModuloExpression signed_right("signed right"_view, s8);
  ModuloExpression unsigned_left("unsigned left"_view, u8);
  ModuloExpression unsigned_right("unsigned right"_view, u8);
  ModuloExpression other("other"_view, u16);
  ModuloExpression unresolved("unresolved"_view, unresolved_type);
  ModuloExpression invalid("invalid"_view, Unknown::get_unknown());
  auto& real = Constants::Real::create_synthetic(domain, r32, 1.0);
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& signed_exact =
      Operations::Modulo::create_synthetic(domain, signed_left, signed_right);
  auto& unsigned_exact = Operations::Modulo::create_synthetic(
      domain, unsigned_left, unsigned_right);
  auto& mismatch =
      Operations::Modulo::create_synthetic(domain, unsigned_left, other);
  auto& unresolved_pair =
      Operations::Modulo::create_synthetic(domain, unresolved, unresolved);
  auto& invalid_pair =
      Operations::Modulo::create_synthetic(domain, invalid, invalid);
  auto& real_values = Operations::Modulo::create_synthetic(domain, real, real);
  auto& flags = Operations::Modulo::create_synthetic(domain, truth, truth);
  auto& byte_values =
      Operations::Modulo::create_synthetic(domain, bytes, bytes);

  EXPECT(signed_exact.get_type().resolve().is<Unknown>());
  EXPECT_NOT(signed_exact.get_anchor());
  EXPECT(link_operation(signed_exact, source));
  EXPECT(link_operation(unsigned_exact, source));
  EXPECT(!link_operation(mismatch, source));
  EXPECT(!link_operation(unresolved_pair, source));
  EXPECT(!link_operation(invalid_pair, source));
  EXPECT(link_operation(real_values, source));
  EXPECT(!link_operation(flags, source));
  EXPECT(!link_operation(byte_values, source));

  auto retained = selected(unsigned_exact.fold());
  auto real_retained = selected(real_values.fold());

  EXPECT(&signed_exact.get_type() == &s8);
  EXPECT(&unsigned_exact.get_type() == &u8);
  EXPECT_NOT(retained);
  ASSERT(real_retained);
  EXPECT(value_is<Constants::Real>(*real_retained, R64(0.0)));
  EXPECT(mismatch.get_type().resolve().is<Unknown>());
  EXPECT(unresolved_pair.get_type().resolve().is<Unknown>());
  EXPECT(invalid_pair.get_type().resolve().is<Unknown>());
  EXPECT(&real_values.get_type() == &r32);
  EXPECT(flags.get_type().resolve().is<Unknown>());
  EXPECT(byte_values.get_type().resolve().is<Unknown>());
}

PERIMORTEM_UNIT_TEST(LibraryModulo, integer_remainders) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::S8 signed_type;
  Types::U8 unsigned_type;
  auto& positive = Constants::Signed::create_synthetic(domain, signed_type, 7);
  auto& negative = Constants::Signed::create_synthetic(domain, signed_type, -7);
  auto& three = Constants::Signed::create_synthetic(domain, signed_type, 3);
  auto& negative_three =
      Constants::Signed::create_synthetic(domain, signed_type, -3);
  auto& zero = Constants::Signed::create_synthetic(domain, signed_type, 0);
  auto& one = Constants::Signed::create_synthetic(domain, signed_type, 1);
  auto& negative_one =
      Constants::Signed::create_synthetic(domain, signed_type, -1);
  auto& minimum =
      Constants::Signed::create_synthetic(domain, signed_type, -128);
  auto& invalid_left =
      Constants::Signed::create_synthetic(domain, signed_type, 255);
  auto& invalid_right =
      Constants::Signed::create_synthetic(domain, signed_type, 256);
  auto& seven = Constants::Unsigned::create_synthetic(domain, unsigned_type, 7);
  auto& unsigned_three =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 3);
  auto& unsigned_zero =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 0);
  auto& unsigned_one =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 1);
  auto& maximum =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 255);
  auto& invalid_unsigned_left =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 256);
  auto& invalid_unsigned_right =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 257);
  auto& positive_result =
      Operations::Modulo::create_synthetic(domain, positive, three);
  auto& negative_dividend =
      Operations::Modulo::create_synthetic(domain, negative, three);
  auto& negative_divisor =
      Operations::Modulo::create_synthetic(domain, positive, negative_three);
  auto& both_negative =
      Operations::Modulo::create_synthetic(domain, negative, negative_three);
  auto& zero_result = Operations::Modulo::create_synthetic(domain, zero, one);
  auto& one_result =
      Operations::Modulo::create_synthetic(domain, positive, one);
  auto& minimum_result =
      Operations::Modulo::create_synthetic(domain, minimum, three);
  auto& zero_divisor =
      Operations::Modulo::create_synthetic(domain, positive, zero);
  auto& endpoint_overflow =
      Operations::Modulo::create_synthetic(domain, minimum, negative_one);
  auto& signed_width =
      Operations::Modulo::create_synthetic(domain, invalid_left, invalid_right);
  auto& unsigned_result =
      Operations::Modulo::create_synthetic(domain, seven, unsigned_three);
  auto& unsigned_zero_result =
      Operations::Modulo::create_synthetic(domain, unsigned_zero, unsigned_one);
  auto& unsigned_endpoint =
      Operations::Modulo::create_synthetic(domain, maximum, unsigned_three);
  auto& unsigned_zero_divisor =
      Operations::Modulo::create_synthetic(domain, maximum, unsigned_zero);
  auto& unsigned_width = Operations::Modulo::create_synthetic(
      domain, invalid_unsigned_left, invalid_unsigned_right);

  EXPECT(positive_result.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(positive_result, source));
  EXPECT(link_operation(negative_dividend, source));
  EXPECT(link_operation(negative_divisor, source));
  EXPECT(link_operation(both_negative, source));
  EXPECT(link_operation(zero_result, source));
  EXPECT(link_operation(one_result, source));
  EXPECT(link_operation(minimum_result, source));
  EXPECT(link_operation(zero_divisor, source));
  EXPECT(link_operation(endpoint_overflow, source));
  EXPECT(link_operation(signed_width, source));
  EXPECT(link_operation(unsigned_result, source));
  EXPECT(link_operation(unsigned_zero_result, source));
  EXPECT(link_operation(unsigned_endpoint, source));
  EXPECT(link_operation(unsigned_zero_divisor, source));
  EXPECT(link_operation(unsigned_width, source));

  auto positive_fold = selected(positive_result.fold());
  auto negative_fold = selected(negative_dividend.fold());
  auto negative_divisor_fold = selected(negative_divisor.fold());
  auto both_negative_fold = selected(both_negative.fold());
  auto zero_fold = selected(zero_result.fold());
  auto one_fold = selected(one_result.fold());
  auto minimum_fold = selected(minimum_result.fold());
  auto unsigned_fold = selected(unsigned_result.fold());
  auto unsigned_zero_fold = selected(unsigned_zero_result.fold());
  auto unsigned_endpoint_fold = selected(unsigned_endpoint.fold());

  ASSERT(
      positive_fold && negative_fold && negative_divisor_fold &&
      both_negative_fold && zero_fold && one_fold && minimum_fold &&
      unsigned_fold && unsigned_zero_fold && unsigned_endpoint_fold);
  EXPECT(value_is<Constants::Signed>(*positive_fold, S64(1)));
  EXPECT(value_is<Constants::Signed>(*negative_fold, S64(-1)));
  EXPECT(value_is<Constants::Signed>(*negative_divisor_fold, S64(1)));
  EXPECT(value_is<Constants::Signed>(*both_negative_fold, S64(-1)));
  EXPECT(value_is<Constants::Signed>(*zero_fold, S64(0)));
  EXPECT(value_is<Constants::Signed>(*one_fold, S64(0)));
  EXPECT(value_is<Constants::Signed>(*minimum_fold, S64(-2)));
  EXPECT(value_is<Constants::Unsigned>(*unsigned_fold, U64(1)));
  EXPECT(value_is<Constants::Unsigned>(*unsigned_zero_fold, U64(0)));
  EXPECT(value_is<Constants::Unsigned>(*unsigned_endpoint_fold, U64(0)));
  EXPECT(&positive_fold->get_type() == &signed_type);
  EXPECT(&unsigned_fold->get_type() == &unsigned_type);
  EXPECT(reports(
      zero_divisor.fold(), Expression::Error::Type::DivisionByZero,
      zero_divisor));
  EXPECT(reports(
      unsigned_zero_divisor.fold(), Expression::Error::Type::DivisionByZero,
      unsigned_zero_divisor));
  EXPECT(reports(
      endpoint_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      endpoint_overflow));
  EXPECT(reports(
      signed_width.fold(), Expression::Error::Type::ArithmeticOverflow,
      signed_width));
  EXPECT(reports(
      unsigned_width.fold(), Expression::Error::Type::ArithmeticOverflow,
      unsigned_width));
}

PERIMORTEM_UNIT_TEST(LibraryModulo, atomic_provenance) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 selected_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, selected_type, 1);
  auto& folded =
      Constants::Unsigned::create_synthetic(domain, selected_type, 13);
  auto& divisor =
      Constants::Unsigned::create_synthetic(domain, selected_type, 5);
  ModuloFoldInput child(domain, input, folded, selected_type);
  ModuloFoldInput failing(domain, input, folded, selected_type, True);
  auto& modulo = Operations::Modulo::create_synthetic(domain, child, divisor);
  auto& failure =
      Operations::Modulo::create_synthetic(domain, failing, divisor);

  EXPECT(modulo.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(modulo, source));
  EXPECT(link_operation(modulo, source));
  EXPECT(link_operation(failure, source));

  auto first = selected(modulo.fold());
  auto second = selected(modulo.fold());

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(value_is<Constants::Unsigned>(*first, U64(3)));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.fold(), Expression::Error::Type::InvalidConstant, failing));

  const auto& parser_type = resolve_library_signed(source, "S64"_view);
  Errors success_errors;
  Tokenizer success_tokens(domain, "-7 % -3"_view, "modulo.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations success_associations(success_tokens.get_arena());
  Cursor success_cursor(success_tokens, success_errors, success_associations);
  Token success_left_trigger = success_cursor.consume();
  Token success_left_end = success_cursor.consume();
  auto success_left_anchor = Anchor::create(
      success_left_trigger, Span(success_left_trigger, success_left_end));
  auto& success_left = Constants::Signed::create_authored(
      domain, parser_type, -7, success_left_anchor);
  auto parsed = Interpreter::Operation::parse_binary(
      Code::Type::ModOp, source, success_cursor, success_left,
      Span(success_left_trigger, success_left_end));
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "-7 % true"_view, "modulo.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations failure_associations(failure_tokens.get_arena());
  Cursor failure_cursor(failure_tokens, failure_errors, failure_associations);
  Token failure_left_trigger = failure_cursor.consume();
  Token failure_left_end = failure_cursor.consume();
  auto failure_left_anchor = Anchor::create(
      failure_left_trigger, Span(failure_left_trigger, failure_left_end));
  auto& failure_left = Constants::Signed::create_authored(
      domain, parser_type, -7, failure_left_anchor);
  auto rejected = Interpreter::Operation::parse_binary(
      Code::Type::ModOp, source, failure_cursor, failure_left,
      Span(failure_left_trigger, failure_left_end));

  ASSERT(parsed);
  EXPECT(parsed->is_identity<Operations::Modulo>());
  EXPECT(parsed->get_type().resolve().is<Unknown>());
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT(parsed->link(success_cursor, source));

  auto parsed_fold = parsed->visit<Operation>(
      [&](Operation& operation) { return selected(operation.fold()); },
      [](Abstract&) -> Option<Tetrodotoxin::Library::Language::Constant&> {
        return {};
      });
  auto parsed_value = parsed_fold
                          ? get_value<Constants::Signed, S64>(*parsed_fold)
                          : Option<S64>();

  ASSERT(parsed_value);
  EXPECT(*parsed_value == -1);
  EXPECT(&parsed->get_type() == &parser_type);

  ASSERT(rejected);
  EXPECT(rejected->is_identity<Operations::Modulo>());
  EXPECT(rejected->get_type().resolve().is<Unknown>());
  EXPECT(failure_cursor.matches(Code::Type::Terminal));
  EXPECT(failure_errors.is_empty());
  EXPECT_NOT(rejected->link(failure_cursor, source));
  EXPECT(rejected->get_type().resolve().is<Unknown>());
  EXPECT_EQ(failure_errors.get_size(), Count(1));
}
