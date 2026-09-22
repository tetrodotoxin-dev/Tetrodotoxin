// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/divide.hpp"

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
#include "tetrodotoxin/library/language/types/r64.hpp"
#include "tetrodotoxin/library/language/types/s8.hpp"
#include "tetrodotoxin/library/language/types/u16.hpp"
#include "tetrodotoxin/library/language/types/u64.hpp"
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

static Harness LibraryDivide = {
  .name = "Tetrodotoxin::Library::Language::Operations::Divide"_view,
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

class DivideExpression : public Expression {
 public:
  DivideExpression(View::Bytes name, const Abstract& type)
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

class DivideUnresolvedType : public Tetrodotoxin::Source::Type {
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

class DivideFoldInput : public Operation {
 public:
  DivideFoldInput(
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

PERIMORTEM_UNIT_TEST(LibraryDivide, type_selection) {
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
  DivideUnresolvedType unresolved_type;
  DivideExpression signed_left("signed left"_view, s8);
  DivideExpression signed_right("signed right"_view, s8);
  DivideExpression unsigned_left("unsigned left"_view, u8);
  DivideExpression unsigned_right("unsigned right"_view, u8);
  DivideExpression real_left("real left"_view, r32);
  DivideExpression real_right("real right"_view, r32);
  DivideExpression other("other"_view, u16);
  DivideExpression unresolved("unresolved"_view, unresolved_type);
  DivideExpression invalid("invalid"_view, Unknown::get_unknown());
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& signed_exact =
      Operations::Divide::create_synthetic(domain, signed_left, signed_right);
  auto& unsigned_exact = Operations::Divide::create_synthetic(
      domain, unsigned_left, unsigned_right);
  auto& real_exact =
      Operations::Divide::create_synthetic(domain, real_left, real_right);
  auto& mismatch =
      Operations::Divide::create_synthetic(domain, unsigned_left, other);
  auto& unresolved_pair =
      Operations::Divide::create_synthetic(domain, unresolved, unresolved);
  auto& invalid_pair =
      Operations::Divide::create_synthetic(domain, invalid, invalid);
  auto& flags = Operations::Divide::create_synthetic(domain, truth, truth);
  auto& byte_values =
      Operations::Divide::create_synthetic(domain, bytes, bytes);

  EXPECT(signed_exact.get_type().resolve().is<Unknown>());
  EXPECT_NOT(signed_exact.get_anchor());
  EXPECT(link_operation(signed_exact, source));
  EXPECT(link_operation(unsigned_exact, source));
  EXPECT(link_operation(real_exact, source));
  EXPECT(!link_operation(mismatch, source));
  EXPECT(!link_operation(unresolved_pair, source));
  EXPECT(!link_operation(invalid_pair, source));
  EXPECT(!link_operation(flags, source));
  EXPECT(!link_operation(byte_values, source));

  auto retained = selected(unsigned_exact.fold());

  EXPECT(&signed_exact.get_type() == &s8);
  EXPECT(&unsigned_exact.get_type() == &u8);
  EXPECT(&real_exact.get_type() == &r32);
  EXPECT_NOT(retained);
  EXPECT(mismatch.get_type().resolve().is<Unknown>());
  EXPECT(unresolved_pair.get_type().resolve().is<Unknown>());
  EXPECT(invalid_pair.get_type().resolve().is<Unknown>());
  EXPECT(flags.get_type().resolve().is<Unknown>());
  EXPECT(byte_values.get_type().resolve().is<Unknown>());
}

PERIMORTEM_UNIT_TEST(LibraryDivide, integer_quotients) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::S8 signed_type;
  Types::U8 unsigned_type;
  auto& positive = Constants::Signed::create_synthetic(domain, signed_type, 7);
  auto& negative = Constants::Signed::create_synthetic(domain, signed_type, -7);
  auto& two = Constants::Signed::create_synthetic(domain, signed_type, 2);
  auto& negative_two =
      Constants::Signed::create_synthetic(domain, signed_type, -2);
  auto& signed_zero =
      Constants::Signed::create_synthetic(domain, signed_type, 0);
  auto& signed_one =
      Constants::Signed::create_synthetic(domain, signed_type, 1);
  auto& negative_one =
      Constants::Signed::create_synthetic(domain, signed_type, -1);
  auto& minimum =
      Constants::Signed::create_synthetic(domain, signed_type, -128);
  auto& invalid_signed =
      Constants::Signed::create_synthetic(domain, signed_type, 128);
  auto& seven = Constants::Unsigned::create_synthetic(domain, unsigned_type, 7);
  auto& unsigned_two =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 2);
  auto& unsigned_zero =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 0);
  auto& unsigned_one =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 1);
  auto& maximum =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 255);
  auto& invalid_unsigned =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 256);
  auto& positive_result =
      Operations::Divide::create_synthetic(domain, positive, two);
  auto& negative_result =
      Operations::Divide::create_synthetic(domain, negative, two);
  auto& opposite_sign =
      Operations::Divide::create_synthetic(domain, positive, negative_two);
  auto& signed_zero_result =
      Operations::Divide::create_synthetic(domain, signed_zero, signed_one);
  auto& minimum_result =
      Operations::Divide::create_synthetic(domain, minimum, signed_one);
  auto& signed_zero_divisor =
      Operations::Divide::create_synthetic(domain, positive, signed_zero);
  auto& endpoint_overflow =
      Operations::Divide::create_synthetic(domain, minimum, negative_one);
  auto& signed_width =
      Operations::Divide::create_synthetic(domain, invalid_signed, signed_one);
  auto& unsigned_result =
      Operations::Divide::create_synthetic(domain, seven, unsigned_two);
  auto& unsigned_zero_result =
      Operations::Divide::create_synthetic(domain, unsigned_zero, unsigned_one);
  auto& unsigned_endpoint =
      Operations::Divide::create_synthetic(domain, maximum, unsigned_one);
  auto& unsigned_zero_divisor =
      Operations::Divide::create_synthetic(domain, maximum, unsigned_zero);
  auto& unsigned_width = Operations::Divide::create_synthetic(
      domain, invalid_unsigned, unsigned_one);

  EXPECT(positive_result.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(positive_result, source));
  EXPECT(link_operation(negative_result, source));
  EXPECT(link_operation(opposite_sign, source));
  EXPECT(link_operation(signed_zero_result, source));
  EXPECT(link_operation(minimum_result, source));
  EXPECT(link_operation(signed_zero_divisor, source));
  EXPECT(link_operation(endpoint_overflow, source));
  EXPECT(link_operation(signed_width, source));
  EXPECT(link_operation(unsigned_result, source));
  EXPECT(link_operation(unsigned_zero_result, source));
  EXPECT(link_operation(unsigned_endpoint, source));
  EXPECT(link_operation(unsigned_zero_divisor, source));
  EXPECT(link_operation(unsigned_width, source));

  auto positive_fold = selected(positive_result.fold());
  auto negative_fold = selected(negative_result.fold());
  auto opposite_fold = selected(opposite_sign.fold());
  auto signed_zero_fold = selected(signed_zero_result.fold());
  auto minimum_fold = selected(minimum_result.fold());
  auto unsigned_fold = selected(unsigned_result.fold());
  auto unsigned_zero_fold = selected(unsigned_zero_result.fold());
  auto unsigned_endpoint_fold = selected(unsigned_endpoint.fold());

  ASSERT(
      positive_fold && negative_fold && opposite_fold && signed_zero_fold &&
      minimum_fold && unsigned_fold && unsigned_zero_fold &&
      unsigned_endpoint_fold);
  EXPECT(value_is<Constants::Signed>(*positive_fold, S64(3)));
  EXPECT(value_is<Constants::Signed>(*negative_fold, S64(-3)));
  EXPECT(value_is<Constants::Signed>(*opposite_fold, S64(-3)));
  EXPECT(value_is<Constants::Signed>(*signed_zero_fold, S64(0)));
  EXPECT(value_is<Constants::Signed>(*minimum_fold, S64(-128)));
  EXPECT(value_is<Constants::Unsigned>(*unsigned_fold, U64(3)));
  EXPECT(value_is<Constants::Unsigned>(*unsigned_zero_fold, U64(0)));
  EXPECT(value_is<Constants::Unsigned>(*unsigned_endpoint_fold, U64(255)));
  EXPECT(&positive_fold->get_type() == &signed_type);
  EXPECT(&unsigned_endpoint_fold->get_type() == &unsigned_type);
  EXPECT(reports(
      signed_zero_divisor.fold(), Expression::Error::Type::DivisionByZero,
      signed_zero_divisor));
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

PERIMORTEM_UNIT_TEST(LibraryDivide, ieee_domains) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::R32 r32;
  Types::R64 r64;
  auto& narrow_seven = Constants::Real::create_synthetic(domain, r32, 7.0);
  auto& narrow_two = Constants::Real::create_synthetic(domain, r32, 2.0);
  auto& narrow_zero = Constants::Real::create_synthetic(domain, r32, 0.0);
  auto& narrow_negative_zero =
      Constants::Real::create_synthetic(domain, r32, -0.0);
  auto& wide_seven = Constants::Real::create_synthetic(domain, r64, 7.0);
  auto& wide_two = Constants::Real::create_synthetic(domain, r64, 2.0);
  auto& wide_zero = Constants::Real::create_synthetic(domain, r64, 0.0);
  auto& wide_negative_zero =
      Constants::Real::create_synthetic(domain, r64, -0.0);
  auto& narrow_finite =
      Operations::Divide::create_synthetic(domain, narrow_seven, narrow_two);
  auto& narrow_signed_zero = Operations::Divide::create_synthetic(
      domain, narrow_negative_zero, narrow_two);
  auto& narrow_infinity = Operations::Divide::create_synthetic(
      domain, narrow_seven, narrow_negative_zero);
  auto& narrow_nan =
      Operations::Divide::create_synthetic(domain, narrow_zero, narrow_zero);
  auto& wide_finite =
      Operations::Divide::create_synthetic(domain, wide_seven, wide_two);
  auto& wide_signed_zero = Operations::Divide::create_synthetic(
      domain, wide_negative_zero, wide_two);
  auto& wide_infinity = Operations::Divide::create_synthetic(
      domain, wide_seven, wide_negative_zero);
  auto& wide_nan =
      Operations::Divide::create_synthetic(domain, wide_zero, wide_zero);

  EXPECT(narrow_finite.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(narrow_finite, source));
  EXPECT(link_operation(narrow_signed_zero, source));
  EXPECT(link_operation(narrow_infinity, source));
  EXPECT(link_operation(narrow_nan, source));
  EXPECT(link_operation(wide_finite, source));
  EXPECT(link_operation(wide_signed_zero, source));
  EXPECT(link_operation(wide_infinity, source));
  EXPECT(link_operation(wide_nan, source));

  auto narrow_value = selected(narrow_finite.fold());
  auto narrow_zero_value = selected(narrow_signed_zero.fold());
  auto narrow_infinite_value = selected(narrow_infinity.fold());
  auto narrow_nan_value = selected(narrow_nan.fold());
  auto wide_value = selected(wide_finite.fold());
  auto wide_zero_value = selected(wide_signed_zero.fold());
  auto wide_infinite_value = selected(wide_infinity.fold());
  auto wide_nan_value = selected(wide_nan.fold());

  ASSERT(
      narrow_value && narrow_zero_value && narrow_infinite_value &&
      narrow_nan_value && wide_value && wide_zero_value &&
      wide_infinite_value && wide_nan_value);
  auto narrow = get_value<Constants::Real, R64>(*narrow_value);
  auto narrow_zero_result = get_value<Constants::Real, R64>(*narrow_zero_value);
  auto narrow_infinite =
      get_value<Constants::Real, R64>(*narrow_infinite_value);
  auto narrow_nan_result = get_value<Constants::Real, R64>(*narrow_nan_value);
  auto wide = get_value<Constants::Real, R64>(*wide_value);
  auto wide_zero_result = get_value<Constants::Real, R64>(*wide_zero_value);
  auto wide_infinite = get_value<Constants::Real, R64>(*wide_infinite_value);
  auto wide_nan_result = get_value<Constants::Real, R64>(*wide_nan_value);

  ASSERT(
      narrow && narrow_zero_result && narrow_infinite && narrow_nan_result &&
      wide && wide_zero_result && wide_infinite && wide_nan_result);
  EXPECT(*narrow == R64(R32(7.0) / R32(2.0)));
  EXPECT(*wide == 3.5);
  EXPECT(*narrow_zero_result == 0.0 && __builtin_signbit(*narrow_zero_result));
  EXPECT(*wide_zero_result == 0.0 && __builtin_signbit(*wide_zero_result));
  EXPECT(
      __builtin_isinf(*narrow_infinite) && __builtin_signbit(*narrow_infinite));
  EXPECT(__builtin_isinf(*wide_infinite) && __builtin_signbit(*wide_infinite));
  EXPECT(__builtin_isnan(*narrow_nan_result));
  EXPECT(__builtin_isnan(*wide_nan_result));
  EXPECT(&narrow_value->get_type() == &r32);
  EXPECT(&wide_value->get_type() == &r64);
}

PERIMORTEM_UNIT_TEST(LibraryDivide, recursive_and_atomic) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 selected_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, selected_type, 1);
  auto& folded =
      Constants::Unsigned::create_synthetic(domain, selected_type, 12);
  auto& divisor =
      Constants::Unsigned::create_synthetic(domain, selected_type, 3);
  DivideFoldInput child(domain, input, folded, selected_type);
  DivideFoldInput failing(domain, input, folded, selected_type, True);
  auto& divide = Operations::Divide::create_synthetic(domain, child, divisor);
  auto& failure =
      Operations::Divide::create_synthetic(domain, failing, divisor);

  EXPECT(divide.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(divide, source));
  EXPECT(link_operation(divide, source));
  EXPECT(link_operation(failure, source));

  auto first = selected(divide.fold());
  auto second = selected(divide.fold());

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(value_is<Constants::Unsigned>(*first, U64(4)));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.fold(), Expression::Error::Type::InvalidConstant, failing));

  const auto& parser_type = resolve_library_unsigned(source, "U64"_view);
  Errors success_errors;
  Tokenizer success_tokens(domain, "24 / 2"_view, "divide.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations success_associations(success_tokens.get_arena());
  Cursor success_cursor(success_tokens, success_errors, success_associations);
  Token success_left_token = success_cursor.consume();
  auto success_left_anchor =
      Anchor::create(success_left_token, Span(success_left_token));
  auto& success_left = Constants::Unsigned::create_authored(
      domain, parser_type, 24, success_left_anchor);
  auto parsed = Interpreter::Operation::parse_binary(
      Code::Type::DivOp, source, success_cursor, success_left,
      Span(success_left_token));
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "24 / true"_view, "divide.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations failure_associations(failure_tokens.get_arena());
  Cursor failure_cursor(failure_tokens, failure_errors, failure_associations);
  Token failure_left_token = failure_cursor.consume();
  auto failure_left_anchor =
      Anchor::create(failure_left_token, Span(failure_left_token));
  auto& failure_left = Constants::Unsigned::create_authored(
      domain, parser_type, 24, failure_left_anchor);
  auto rejected = Interpreter::Operation::parse_binary(
      Code::Type::DivOp, source, failure_cursor, failure_left,
      Span(failure_left_token));

  ASSERT(parsed);
  EXPECT(parsed->is_identity<Operations::Divide>());
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
                          ? get_value<Constants::Unsigned, U64>(*parsed_fold)
                          : Option<U64>();

  ASSERT(parsed_value);
  EXPECT(*parsed_value == 12);
  EXPECT(&parsed->get_type() == &parser_type);

  ASSERT(rejected);
  EXPECT(rejected->is_identity<Operations::Divide>());
  EXPECT(rejected->get_type().resolve().is<Unknown>());
  EXPECT(failure_cursor.matches(Code::Type::Terminal));
  EXPECT(failure_errors.is_empty());
  EXPECT_NOT(rejected->link(failure_cursor, source));
  EXPECT(rejected->get_type().resolve().is<Unknown>());
  EXPECT_EQ(failure_errors.get_size(), Count(1));
}
