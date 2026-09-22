// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/not_equal.hpp"

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

static Harness LibraryNotEqual = {
  .name = "Tetrodotoxin::Library::Language::Operations::NotEqual"_view,
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

class NotEqualExpression : public Expression {
 public:
  NotEqualExpression(View::Bytes name, const Abstract& type)
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

class NotEqualUnresolvedType : public Tetrodotoxin::Source::Type {
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

class NotEqualFoldInput : public Operation {
 public:
  NotEqualFoldInput(
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

PERIMORTEM_UNIT_TEST(LibraryNotEqual, type_selection) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::S8 s8;
  Types::U8 u8;
  Types::U16 u16;
  Types::R64 r64;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[U8,1]"_view, resolve_library_unsigned(source, "U8"_view), 1);
  Types::Fixed other_bytes_type(
      "Fixed[U8,2]"_view, resolve_library_unsigned(source, "U8"_view), 2);
  NotEqualUnresolvedType unresolved_type;
  NotEqualExpression signed_left("signed left"_view, s8);
  NotEqualExpression signed_right("signed right"_view, s8);
  NotEqualExpression unsigned_left("unsigned left"_view, u8);
  NotEqualExpression unsigned_right("unsigned right"_view, u8);
  NotEqualExpression real_left("real left"_view, r64);
  NotEqualExpression real_right("real right"_view, r64);
  NotEqualExpression flag_left("flag left"_view, boolean);
  NotEqualExpression flag_right("flag right"_view, boolean);
  NotEqualExpression other("other"_view, u16);
  NotEqualExpression unresolved("unresolved"_view, unresolved_type);
  NotEqualExpression invalid("invalid"_view, Unknown::get_unknown());
  NotEqualExpression dynamic_bytes("dynamic bytes"_view, bytes_type);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& same_bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& other_bytes =
      Constants::Bytes::create_synthetic(domain, other_bytes_type, "xx"_view);
  auto& signed_exact =
      Operations::NotEqual::create_synthetic(domain, signed_left, signed_right);
  auto& unsigned_exact = Operations::NotEqual::create_synthetic(
      domain, unsigned_left, unsigned_right);
  auto& real_exact =
      Operations::NotEqual::create_synthetic(domain, real_left, real_right);
  auto& flag_exact =
      Operations::NotEqual::create_synthetic(domain, flag_left, flag_right);
  auto& byte_values =
      Operations::NotEqual::create_synthetic(domain, bytes, same_bytes);
  auto& mismatch =
      Operations::NotEqual::create_synthetic(domain, unsigned_left, other);
  auto& unresolved_pair =
      Operations::NotEqual::create_synthetic(domain, unresolved, unresolved);
  auto& invalid_pair =
      Operations::NotEqual::create_synthetic(domain, invalid, invalid);
  auto& incomplete_bytes =
      Operations::NotEqual::create_synthetic(domain, dynamic_bytes, bytes);
  auto& byte_mismatch =
      Operations::NotEqual::create_synthetic(domain, bytes, other_bytes);

  EXPECT(signed_exact.get_type().resolve().is<Unknown>());
  EXPECT_NOT(signed_exact.get_anchor());
  EXPECT(link_operation(signed_exact, source));
  EXPECT(link_operation(unsigned_exact, source));
  EXPECT(link_operation(real_exact, source));
  EXPECT(link_operation(flag_exact, source));
  EXPECT(link_operation(byte_values, source));
  EXPECT(!link_operation(mismatch, source));
  EXPECT(!link_operation(unresolved_pair, source));
  EXPECT(!link_operation(invalid_pair, source));
  EXPECT(!link_operation(incomplete_bytes, source));
  EXPECT(!link_operation(byte_mismatch, source));

  auto retained = selected(unsigned_exact.fold());

  EXPECT(&signed_exact.get_type() == &resolve_library_flag(source));
  EXPECT(&unsigned_exact.get_type() == &resolve_library_flag(source));
  EXPECT(&real_exact.get_type() == &resolve_library_flag(source));
  EXPECT(&flag_exact.get_type() == &resolve_library_flag(source));
  EXPECT(&byte_values.get_type() == &resolve_library_flag(source));
  EXPECT_NOT(retained);
  EXPECT(mismatch.get_type().resolve().is<Unknown>());
  EXPECT(unresolved_pair.get_type().resolve().is<Unknown>());
  EXPECT(invalid_pair.get_type().resolve().is<Unknown>());
  EXPECT(incomplete_bytes.get_type().resolve().is<Unknown>());
  EXPECT(byte_mismatch.get_type().resolve().is<Unknown>());
}

PERIMORTEM_UNIT_TEST(LibraryNotEqual, constant_domains) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::S8 signed_type;
  Types::U8 unsigned_type;
  Types::R64 real_type;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[U8,2]"_view, resolve_library_unsigned(source, "U8"_view), 2);
  auto& signed_value =
      Constants::Signed::create_synthetic(domain, signed_type, -8);
  auto& same_signed =
      Constants::Signed::create_synthetic(domain, signed_type, -8);
  auto& other_signed =
      Constants::Signed::create_synthetic(domain, signed_type, 8);
  auto& unsigned_value =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 8);
  auto& same_unsigned =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 8);
  auto& other_unsigned =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 9);
  auto& real_value = Constants::Real::create_synthetic(domain, real_type, 0.5);
  auto& same_real = Constants::Real::create_synthetic(domain, real_type, 0.5);
  auto& other_real = Constants::Real::create_synthetic(domain, real_type, 1.0);
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& same_truth = Constants::True::create_synthetic(domain, boolean);
  auto& falsity = Constants::False::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "ab"_view);
  auto& same_bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "ab"_view);
  auto& other_bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "ac"_view);
  auto& signed_same =
      Operations::NotEqual::create_synthetic(domain, signed_value, same_signed);
  auto& signed_different = Operations::NotEqual::create_synthetic(
      domain, signed_value, other_signed);
  auto& unsigned_same = Operations::NotEqual::create_synthetic(
      domain, unsigned_value, same_unsigned);
  auto& unsigned_different = Operations::NotEqual::create_synthetic(
      domain, unsigned_value, other_unsigned);
  auto& real_same =
      Operations::NotEqual::create_synthetic(domain, real_value, same_real);
  auto& real_different =
      Operations::NotEqual::create_synthetic(domain, real_value, other_real);
  auto& flag_same =
      Operations::NotEqual::create_synthetic(domain, truth, same_truth);
  auto& flag_different =
      Operations::NotEqual::create_synthetic(domain, truth, falsity);
  auto& bytes_same =
      Operations::NotEqual::create_synthetic(domain, bytes, same_bytes);
  auto& bytes_different =
      Operations::NotEqual::create_synthetic(domain, bytes, other_bytes);

  EXPECT(signed_same.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(signed_same, source));
  EXPECT(link_operation(signed_different, source));
  EXPECT(link_operation(unsigned_same, source));
  EXPECT(link_operation(unsigned_different, source));
  EXPECT(link_operation(real_same, source));
  EXPECT(link_operation(real_different, source));
  EXPECT(link_operation(flag_same, source));
  EXPECT(link_operation(flag_different, source));
  EXPECT(link_operation(bytes_same, source));
  EXPECT(link_operation(bytes_different, source));

  auto signed_no = selected(signed_same.fold());
  auto signed_yes = selected(signed_different.fold());
  auto unsigned_no = selected(unsigned_same.fold());
  auto unsigned_yes = selected(unsigned_different.fold());
  auto real_no = selected(real_same.fold());
  auto real_yes = selected(real_different.fold());
  auto flag_no = selected(flag_same.fold());
  auto flag_yes = selected(flag_different.fold());
  auto bytes_no = selected(bytes_same.fold());
  auto bytes_yes = selected(bytes_different.fold());

  ASSERT(
      signed_no && signed_yes && unsigned_no && unsigned_yes && real_no &&
      real_yes && flag_no && flag_yes && bytes_no && bytes_yes);
  EXPECT(signed_no->is_identity<Constants::False>());
  EXPECT(signed_yes->is_identity<Constants::True>());
  EXPECT(unsigned_no->is_identity<Constants::False>());
  EXPECT(unsigned_yes->is_identity<Constants::True>());
  EXPECT(real_no->is_identity<Constants::False>());
  EXPECT(real_yes->is_identity<Constants::True>());
  EXPECT(flag_no->is_identity<Constants::False>());
  EXPECT(flag_yes->is_identity<Constants::True>());
  EXPECT(bytes_no->is_identity<Constants::False>());
  EXPECT(bytes_yes->is_identity<Constants::True>());
}

PERIMORTEM_UNIT_TEST(LibraryNotEqual, real_inverse) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::R64 real_type;
  auto& left_nan = Constants::Real::create_synthetic(
      domain, real_type, __builtin_nan("left"));
  auto& right_nan = Constants::Real::create_synthetic(
      domain, real_type, __builtin_nan("right"));
  auto& finite = Constants::Real::create_synthetic(domain, real_type, 1.0);
  auto& positive_zero =
      Constants::Real::create_synthetic(domain, real_type, 0.0);
  auto& negative_zero =
      Constants::Real::create_synthetic(domain, real_type, -0.0);
  auto& nan_pair =
      Operations::NotEqual::create_synthetic(domain, left_nan, right_nan);
  auto& nan_finite =
      Operations::NotEqual::create_synthetic(domain, left_nan, finite);
  auto& signed_zero = Operations::NotEqual::create_synthetic(
      domain, positive_zero, negative_zero);

  EXPECT(nan_pair.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(nan_pair, source));
  EXPECT(link_operation(nan_finite, source));
  EXPECT(link_operation(signed_zero, source));

  auto nan_same = selected(nan_pair.fold());
  auto nan_other = selected(nan_finite.fold());
  auto zeros = selected(signed_zero.fold());

  ASSERT(nan_same && nan_other && zeros);
  EXPECT(nan_same->is_identity<Constants::False>());
  EXPECT(nan_other->is_identity<Constants::True>());
  EXPECT(zeros->is_identity<Constants::False>());
}

PERIMORTEM_UNIT_TEST(LibraryNotEqual, atomic_provenance) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 selected_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, selected_type, 1);
  auto& folded =
      Constants::Unsigned::create_synthetic(domain, selected_type, 8);
  auto& right = Constants::Unsigned::create_synthetic(domain, selected_type, 7);
  auto& wrong_domain =
      Constants::Bytes::create_synthetic(domain, selected_type, "x"_view);
  NotEqualFoldInput child(domain, input, folded, selected_type);
  NotEqualFoldInput failing(domain, input, folded, selected_type, True);
  NotEqualFoldInput invalid_child(domain, input, wrong_domain, selected_type);
  auto& not_equal =
      Operations::NotEqual::create_synthetic(domain, child, right);
  auto& failure =
      Operations::NotEqual::create_synthetic(domain, failing, right);
  auto& invalid_constant =
      Operations::NotEqual::create_synthetic(domain, invalid_child, right);

  EXPECT(not_equal.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(not_equal, source));
  EXPECT(link_operation(not_equal, source));
  EXPECT(link_operation(failure, source));
  EXPECT(link_operation(invalid_constant, source));

  auto first = selected(not_equal.fold());
  auto second = selected(not_equal.fold());

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(first->is_identity<Constants::True>());
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.fold(), Expression::Error::Type::InvalidConstant, failing));
  EXPECT(reports(
      invalid_constant.fold(), Expression::Error::Type::InvalidConstant,
      invalid_child));

  const auto& parser_type = resolve_library_unsigned(source, "U64"_view);
  Errors success_errors;
  Tokenizer success_tokens(domain, "2 != 2"_view, "not_equal.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations success_associations(success_tokens.get_arena());
  Cursor success_cursor(success_tokens, success_errors, success_associations);
  Token success_left_token = success_cursor.consume();
  auto success_left_anchor =
      Anchor::create(success_left_token, Span(success_left_token));
  auto& success_left = Constants::Unsigned::create_authored(
      domain, parser_type, 2, success_left_anchor);
  auto parsed = Interpreter::Operation::parse_binary(
      Code::Type::NotEqOp, source, success_cursor, success_left,
      Span(success_left_token));
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "2 != true"_view, "not_equal.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations failure_associations(failure_tokens.get_arena());
  Cursor failure_cursor(failure_tokens, failure_errors, failure_associations);
  Token failure_left_token = failure_cursor.consume();
  auto failure_left_anchor =
      Anchor::create(failure_left_token, Span(failure_left_token));
  auto& failure_left = Constants::Unsigned::create_authored(
      domain, parser_type, 2, failure_left_anchor);
  auto rejected = Interpreter::Operation::parse_binary(
      Code::Type::NotEqOp, source, failure_cursor, failure_left,
      Span(failure_left_token));

  ASSERT(parsed);
  EXPECT(parsed->is_identity<Operations::NotEqual>());
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
  EXPECT(parsed_fold->is_identity<Constants::False>());
  EXPECT(&parsed->get_type() == &resolve_library_flag(source));

  ASSERT(rejected);
  EXPECT(rejected->is_identity<Operations::NotEqual>());
  EXPECT(rejected->get_type().resolve().is<Unknown>());
  EXPECT(failure_cursor.matches(Code::Type::Terminal));
  EXPECT(failure_errors.is_empty());
  EXPECT_NOT(rejected->link(failure_cursor, source));
  EXPECT(rejected->get_type().resolve().is<Unknown>());
  EXPECT_EQ(failure_errors.get_size(), Count(1));
}
