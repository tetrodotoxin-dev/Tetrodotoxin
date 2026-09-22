// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/greater_equal.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/interpreter/operation.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
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

static Harness LibraryGreaterEqual = {
  .name = "Tetrodotoxin::Library::Language::Operations::GreaterEqual"_view,
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

class GreaterEqualExpression : public Expression {
 public:
  GreaterEqualExpression(View::Bytes name, const Abstract& type)
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

class GreaterEqualUnresolvedType : public Tetrodotoxin::Source::Type {
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

class GreaterEqualFoldInput : public Operation {
 public:
  GreaterEqualFoldInput(
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

PERIMORTEM_UNIT_TEST(LibraryGreaterEqual, type_selection) {
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
  GreaterEqualUnresolvedType unresolved_type;
  GreaterEqualExpression signed_left("signed left"_view, s8);
  GreaterEqualExpression signed_right("signed right"_view, s8);
  GreaterEqualExpression unsigned_left("unsigned left"_view, u8);
  GreaterEqualExpression unsigned_right("unsigned right"_view, u8);
  GreaterEqualExpression real_left("real left"_view, r32);
  GreaterEqualExpression real_right("real right"_view, r32);
  GreaterEqualExpression other("other"_view, u16);
  GreaterEqualExpression unresolved("unresolved"_view, unresolved_type);
  GreaterEqualExpression invalid("invalid"_view, Unknown::get_unknown());
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& signed_exact = Operations::GreaterEqual::create_synthetic(
      domain, signed_left, signed_right);
  auto& unsigned_exact = Operations::GreaterEqual::create_synthetic(
      domain, unsigned_left, unsigned_right);
  auto& real_exact =
      Operations::GreaterEqual::create_synthetic(domain, real_left, real_right);
  auto& mismatch =
      Operations::GreaterEqual::create_synthetic(domain, unsigned_left, other);
  auto& unresolved_pair = Operations::GreaterEqual::create_synthetic(
      domain, unresolved, unresolved);
  auto& invalid_pair =
      Operations::GreaterEqual::create_synthetic(domain, invalid, invalid);
  auto& flags =
      Operations::GreaterEqual::create_synthetic(domain, truth, truth);
  auto& byte_values =
      Operations::GreaterEqual::create_synthetic(domain, bytes, bytes);

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

  EXPECT(&signed_exact.get_type() == &resolve_library_flag(source));
  EXPECT(&unsigned_exact.get_type() == &resolve_library_flag(source));
  EXPECT(&real_exact.get_type() == &resolve_library_flag(source));
  EXPECT_NOT(retained);
  EXPECT(mismatch.get_type().resolve().is<Unknown>());
  EXPECT(unresolved_pair.get_type().resolve().is<Unknown>());
  EXPECT(invalid_pair.get_type().resolve().is<Unknown>());
  EXPECT(flags.get_type().resolve().is<Unknown>());
  EXPECT(byte_values.get_type().resolve().is<Unknown>());
}

PERIMORTEM_UNIT_TEST(LibraryGreaterEqual, integer_endpoints) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::S8 signed_type;
  Types::U8 unsigned_type;
  auto& minimum =
      Constants::Signed::create_synthetic(domain, signed_type, -128);
  auto& maximum = Constants::Signed::create_synthetic(domain, signed_type, 127);
  auto& equal = Constants::Signed::create_synthetic(domain, signed_type, 127);
  auto& zero = Constants::Unsigned::create_synthetic(domain, unsigned_type, 0);
  auto& top = Constants::Unsigned::create_synthetic(domain, unsigned_type, 255);
  auto& same_top =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 255);
  auto& signed_true =
      Operations::GreaterEqual::create_synthetic(domain, maximum, minimum);
  auto& signed_false =
      Operations::GreaterEqual::create_synthetic(domain, minimum, maximum);
  auto& signed_equal =
      Operations::GreaterEqual::create_synthetic(domain, maximum, equal);
  auto& unsigned_true =
      Operations::GreaterEqual::create_synthetic(domain, top, zero);
  auto& unsigned_false =
      Operations::GreaterEqual::create_synthetic(domain, zero, top);
  auto& unsigned_equal =
      Operations::GreaterEqual::create_synthetic(domain, top, same_top);

  EXPECT(signed_true.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(signed_true, source));
  EXPECT(link_operation(signed_false, source));
  EXPECT(link_operation(signed_equal, source));
  EXPECT(link_operation(unsigned_true, source));
  EXPECT(link_operation(unsigned_false, source));
  EXPECT(link_operation(unsigned_equal, source));

  auto signed_yes = selected(signed_true.fold());
  auto signed_no = selected(signed_false.fold());
  auto signed_same = selected(signed_equal.fold());
  auto unsigned_yes = selected(unsigned_true.fold());
  auto unsigned_no = selected(unsigned_false.fold());
  auto unsigned_same = selected(unsigned_equal.fold());

  ASSERT(
      signed_yes && signed_no && signed_same && unsigned_yes && unsigned_no &&
      unsigned_same);
  EXPECT(signed_yes->is_identity<Constants::True>());
  EXPECT(signed_no->is_identity<Constants::False>());
  EXPECT(signed_same->is_identity<Constants::True>());
  EXPECT(unsigned_yes->is_identity<Constants::True>());
  EXPECT(unsigned_no->is_identity<Constants::False>());
  EXPECT(unsigned_same->is_identity<Constants::True>());
}

PERIMORTEM_UNIT_TEST(LibraryGreaterEqual, ieee_domains) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::R32 r32;
  Types::R64 r64;
  auto& narrow_left = Constants::Real::create_synthetic(domain, r32, 1.0);
  auto& narrow_right =
      Constants::Real::create_synthetic(domain, r32, 1.00000001);
  auto& wide_left = Constants::Real::create_synthetic(domain, r64, 4.0);
  auto& wide_right = Constants::Real::create_synthetic(domain, r64, 3.0);
  auto& positive_infinity =
      Constants::Real::create_synthetic(domain, r64, __builtin_inf());
  auto& negative_infinity =
      Constants::Real::create_synthetic(domain, r64, -__builtin_inf());
  auto& nan = Constants::Real::create_synthetic(domain, r64, __builtin_nan(""));
  auto& positive_zero = Constants::Real::create_synthetic(domain, r64, 0.0);
  auto& negative_zero = Constants::Real::create_synthetic(domain, r64, -0.0);
  auto& narrow = Operations::GreaterEqual::create_synthetic(
      domain, narrow_left, narrow_right);
  auto& wide =
      Operations::GreaterEqual::create_synthetic(domain, wide_left, wide_right);
  auto& positive_infinite = Operations::GreaterEqual::create_synthetic(
      domain, positive_infinity, wide_left);
  auto& negative_infinite = Operations::GreaterEqual::create_synthetic(
      domain, negative_infinity, wide_left);
  auto& left_unordered =
      Operations::GreaterEqual::create_synthetic(domain, nan, wide_left);
  auto& right_unordered =
      Operations::GreaterEqual::create_synthetic(domain, wide_left, nan);
  auto& zero_forward = Operations::GreaterEqual::create_synthetic(
      domain, positive_zero, negative_zero);
  auto& zero_reverse = Operations::GreaterEqual::create_synthetic(
      domain, negative_zero, positive_zero);

  EXPECT(narrow.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(narrow, source));
  EXPECT(link_operation(wide, source));
  EXPECT(link_operation(positive_infinite, source));
  EXPECT(link_operation(negative_infinite, source));
  EXPECT(link_operation(left_unordered, source));
  EXPECT(link_operation(right_unordered, source));
  EXPECT(link_operation(zero_forward, source));
  EXPECT(link_operation(zero_reverse, source));

  auto narrow_result = selected(narrow.fold());
  auto wide_result = selected(wide.fold());
  auto positive_result = selected(positive_infinite.fold());
  auto negative_result = selected(negative_infinite.fold());
  auto left_nan = selected(left_unordered.fold());
  auto right_nan = selected(right_unordered.fold());
  auto forward = selected(zero_forward.fold());
  auto reverse = selected(zero_reverse.fold());

  ASSERT(
      narrow_result && wide_result && positive_result && negative_result &&
      left_nan && right_nan && forward && reverse);
  EXPECT(narrow_result->is_identity<Constants::True>());
  EXPECT(wide_result->is_identity<Constants::True>());
  EXPECT(positive_result->is_identity<Constants::True>());
  EXPECT(negative_result->is_identity<Constants::False>());
  EXPECT(left_nan->is_identity<Constants::False>());
  EXPECT(right_nan->is_identity<Constants::False>());
  EXPECT(forward->is_identity<Constants::True>());
  EXPECT(reverse->is_identity<Constants::True>());
  EXPECT(&narrow_result->get_type() == &resolve_library_flag(source));
}

PERIMORTEM_UNIT_TEST(LibraryGreaterEqual, atomic_provenance) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 selected_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, selected_type, 1);
  auto& folded =
      Constants::Unsigned::create_synthetic(domain, selected_type, 8);
  auto& right = Constants::Unsigned::create_synthetic(domain, selected_type, 8);
  GreaterEqualFoldInput child(domain, input, folded, selected_type);
  GreaterEqualFoldInput failing(domain, input, folded, selected_type, True);
  auto& greater_equal =
      Operations::GreaterEqual::create_synthetic(domain, child, right);
  auto& failure =
      Operations::GreaterEqual::create_synthetic(domain, failing, right);

  EXPECT(greater_equal.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(greater_equal, source));
  EXPECT(link_operation(greater_equal, source));
  EXPECT(link_operation(failure, source));

  auto first = selected(greater_equal.fold());
  auto second = selected(greater_equal.fold());

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(first->is_identity<Constants::True>());
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.fold(), Expression::Error::Type::InvalidConstant, failing));

  const auto& parser_type = resolve_library_unsigned(source, "U64"_view);
  Errors success_errors;
  Tokenizer success_tokens(domain, "2 >= 2"_view, "greater-equal.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations success_associations(success_tokens.get_arena());
  Cursor success_cursor(success_tokens, success_errors, success_associations);
  Token success_left_token = success_cursor.consume();
  auto success_left_anchor =
      Anchor::create(success_left_token, Span(success_left_token));
  auto& success_left = Constants::Unsigned::create_authored(
      domain, parser_type, 2, success_left_anchor);
  auto parsed = Interpreter::Operation::parse_binary(
      Code::Type::GreaterEqOp, source, success_cursor, success_left,
      Span(success_left_token));
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "2 >= true"_view, "greater-equal.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations failure_associations(failure_tokens.get_arena());
  Cursor failure_cursor(failure_tokens, failure_errors, failure_associations);
  Token failure_left_token = failure_cursor.consume();
  auto failure_left_anchor =
      Anchor::create(failure_left_token, Span(failure_left_token));
  auto& failure_left = Constants::Unsigned::create_authored(
      domain, parser_type, 2, failure_left_anchor);
  auto rejected = Interpreter::Operation::parse_binary(
      Code::Type::GreaterEqOp, source, failure_cursor, failure_left,
      Span(failure_left_token));

  ASSERT(parsed);
  EXPECT(parsed->is_identity<Operations::GreaterEqual>());
  EXPECT(parsed->get_type().resolve().is<Unknown>());
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT(parsed->link(success_cursor, source));

  auto parsed_fold = parsed->visit<Operation>(
      [&](Operation& operation) { return selected(operation.fold()); },
      [](Abstract&) -> Option<Tetrodotoxin::Library::Language::Constant&> {
        return {};
      });

  ASSERT(parsed_fold);
  EXPECT(parsed_fold->is_identity<Constants::True>());
  EXPECT(&parsed->get_type() == &resolve_library_flag(source));

  ASSERT(rejected);
  EXPECT(rejected->is_identity<Operations::GreaterEqual>());
  EXPECT(rejected->get_type().resolve().is<Unknown>());
  EXPECT(failure_cursor.matches(Code::Type::Terminal));
  EXPECT(failure_errors.is_empty());
  EXPECT_NOT(rejected->link(failure_cursor, source));
  EXPECT(rejected->get_type().resolve().is<Unknown>());
  EXPECT_EQ(failure_errors.get_size(), Count(1));
}
