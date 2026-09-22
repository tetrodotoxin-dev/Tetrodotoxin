// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/subtract.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/operations/multiply.hpp"
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
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness LibrarySubtract = {
  .name = "Tetrodotoxin::Library::Language::Operations::Subtract"_view,
};

static auto link_operation(Operation& operation, const Abstract& context)
    -> Bool {
  Allocator::Arena transaction;
  Tetrodotoxin::Source::Lexical::Errors errors;
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(transaction, {}, "<operation>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Tetrodotoxin::Source::Lexical::Cursor cursor(tokenizer, errors, associations);
  return operation.link(cursor, context);
}

class SubtractExpression : public Expression {
 public:
  SubtractExpression(View::Bytes name, const Abstract& type)
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

static auto get_unsigned(
    const Tetrodotoxin::Library::Language::Constant& expression)
    -> Option<U64> {
  return expression.visit<Constants::Unsigned>(
      [](const Constants::Unsigned& selected) -> Option<U64> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<U64> { return {}; });
}

static auto get_signed(
    const Tetrodotoxin::Library::Language::Constant& expression)
    -> Option<S64> {
  return expression.visit<Constants::Signed>(
      [](const Constants::Signed& selected) -> Option<S64> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<S64> { return {}; });
}

static auto get_real(
    const Tetrodotoxin::Library::Language::Constant& expression)
    -> Option<R64> {
  return expression.visit<Constants::Real>(
      [](const Constants::Real& selected) -> Option<R64> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<R64> { return {}; });
}

PERIMORTEM_UNIT_TEST(LibrarySubtract, type_selection) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 u8;
  Types::U16 u16;
  Types::U64 u64;
  Types::S8 s8;
  Types::R32 r32;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[U8,1]"_view, resolve_library_unsigned(source, "U8"_view), 1);
  SubtractExpression left("left"_view, u8);
  SubtractExpression same("same"_view, u8);
  SubtractExpression other("other"_view, u16);
  SubtractExpression signed_left("signed"_view, s8);
  SubtractExpression signed_right("signed right"_view, s8);
  SubtractExpression real_left("real"_view, r32);
  SubtractExpression real_right("real right"_view, r32);
  SubtractExpression unresolved("unresolved"_view, Unknown::get_unknown());
  auto& wide_constant = Constants::Unsigned::create_synthetic(domain, u64, 12);
  auto& other_constant = Constants::Unsigned::create_synthetic(domain, u16, 12);
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& exact = Operations::Subtract::create_synthetic(domain, left, same);
  auto& mixed_left =
      Operations::Subtract::create_synthetic(domain, wide_constant, left);
  auto& mismatch = Operations::Subtract::create_synthetic(domain, left, other);
  auto& mixed_constants = Operations::Subtract::create_synthetic(
      domain, wide_constant, other_constant);
  auto& signed_exact =
      Operations::Subtract::create_synthetic(domain, signed_left, signed_right);
  auto& real_exact =
      Operations::Subtract::create_synthetic(domain, real_left, real_right);
  auto& flags = Operations::Subtract::create_synthetic(domain, truth, truth);
  auto& byte_values =
      Operations::Subtract::create_synthetic(domain, bytes, bytes);
  auto& invalid =
      Operations::Subtract::create_synthetic(domain, unresolved, same);

  EXPECT(exact.get_type().resolve().is<Unknown>());
  EXPECT_NOT(exact.get_anchor());
  EXPECT(link_operation(exact, source));
  EXPECT(!link_operation(mixed_left, source));
  EXPECT(!link_operation(mismatch, source));
  EXPECT(!link_operation(mixed_constants, source));
  EXPECT(link_operation(signed_exact, source));
  EXPECT(link_operation(real_exact, source));
  EXPECT(!link_operation(flags, source));
  EXPECT(!link_operation(byte_values, source));
  EXPECT(!link_operation(invalid, source));

  auto exact_result = selected(exact.fold());

  EXPECT(&exact.get_type() == &u8);
  EXPECT(mixed_left.get_type().resolve().is<Unknown>());
  EXPECT(&signed_exact.get_type() == &s8);
  EXPECT(&real_exact.get_type() == &r32);
  EXPECT_NOT(exact_result);
  EXPECT(mismatch.get_type().resolve().is<Unknown>());
  EXPECT(mixed_constants.get_type().resolve().is<Unknown>());
  EXPECT(flags.get_type().resolve().is<Unknown>());
  EXPECT(byte_values.get_type().resolve().is<Unknown>());
  EXPECT(invalid.get_type().resolve().is<Unknown>());
}

PERIMORTEM_UNIT_TEST(LibrarySubtract, integer_widths) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 unsigned_type;
  Types::S8 signed_type;
  auto& maximum =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 255);
  auto& one_unsigned =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 1);
  auto& zero_unsigned =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 0);
  auto& maximum_signed =
      Constants::Signed::create_synthetic(domain, signed_type, 127);
  auto& minimum_signed =
      Constants::Signed::create_synthetic(domain, signed_type, -128);
  auto& negative_one =
      Constants::Signed::create_synthetic(domain, signed_type, -1);
  auto& one_signed =
      Constants::Signed::create_synthetic(domain, signed_type, 1);
  auto& zero_signed =
      Constants::Signed::create_synthetic(domain, signed_type, 0);
  auto& negative_twelve =
      Constants::Signed::create_synthetic(domain, signed_type, -12);
  auto& negative_ten =
      Constants::Signed::create_synthetic(domain, signed_type, -10);
  auto& unsigned_success =
      Operations::Subtract::create_synthetic(domain, maximum, one_unsigned);
  auto& unsigned_underflow = Operations::Subtract::create_synthetic(
      domain, zero_unsigned, one_unsigned);
  auto& signed_difference = Operations::Subtract::create_synthetic(
      domain, negative_twelve, negative_ten);
  auto& upper_endpoint = Operations::Subtract::create_synthetic(
      domain, maximum_signed, zero_signed);
  auto& lower_endpoint = Operations::Subtract::create_synthetic(
      domain, minimum_signed, zero_signed);
  auto& signed_overflow = Operations::Subtract::create_synthetic(
      domain, maximum_signed, negative_one);
  auto& signed_underflow = Operations::Subtract::create_synthetic(
      domain, minimum_signed, one_signed);

  EXPECT(unsigned_success.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(unsigned_success, source));
  EXPECT(link_operation(unsigned_underflow, source));
  EXPECT(link_operation(signed_difference, source));
  EXPECT(link_operation(upper_endpoint, source));
  EXPECT(link_operation(lower_endpoint, source));
  EXPECT(link_operation(signed_overflow, source));
  EXPECT(link_operation(signed_underflow, source));

  auto unsigned_value = selected(unsigned_success.fold());
  auto signed_value = selected(signed_difference.fold());
  auto upper_value = selected(upper_endpoint.fold());
  auto lower_value = selected(lower_endpoint.fold());
  auto unsigned_error = unsigned_underflow.fold();
  auto overflow_error = signed_overflow.fold();
  auto underflow_error = signed_underflow.fold();
  auto unsigned_number =
      unsigned_value ? get_unsigned(*unsigned_value) : Option<U64>();
  auto signed_number = signed_value ? get_signed(*signed_value) : Option<S64>();
  auto upper_number = upper_value ? get_signed(*upper_value) : Option<S64>();
  auto lower_number = lower_value ? get_signed(*lower_value) : Option<S64>();

  ASSERT(unsigned_value && signed_value && upper_value && lower_value);
  EXPECT(unsigned_number && *unsigned_number == 254);
  EXPECT(&unsigned_value->get_type() == &unsigned_type);
  EXPECT(signed_number && *signed_number == -2);
  EXPECT(upper_number && *upper_number == 127);
  EXPECT(lower_number && *lower_number == -128);
  EXPECT(reports(
      unsigned_error, Expression::Error::Type::ArithmeticOverflow,
      unsigned_underflow));
  EXPECT(reports(
      overflow_error, Expression::Error::Type::ArithmeticOverflow,
      signed_overflow));
  EXPECT(reports(
      underflow_error, Expression::Error::Type::ArithmeticOverflow,
      signed_underflow));
}

PERIMORTEM_UNIT_TEST(LibrarySubtract, ieee_real_domains) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::R32 r32;
  Types::R64 r64;
  auto& narrow_left = Constants::Real::create_synthetic(domain, r32, R64(4.4));
  auto& narrow_right = Constants::Real::create_synthetic(domain, r32, R64(1.1));
  auto& wide_left = Constants::Real::create_synthetic(domain, r64, R64(-2.5));
  auto& wide_right = Constants::Real::create_synthetic(domain, r64, R64(4.0));
  auto& infinity =
      Constants::Real::create_synthetic(domain, r64, __builtin_inf());
  auto& negative_infinity =
      Constants::Real::create_synthetic(domain, r64, -__builtin_inf());
  auto& nan = Constants::Real::create_synthetic(domain, r64, __builtin_nan(""));
  auto& one = Constants::Real::create_synthetic(domain, r64, R64(1.0));
  auto& narrow =
      Operations::Subtract::create_synthetic(domain, narrow_left, narrow_right);
  auto& wide =
      Operations::Subtract::create_synthetic(domain, wide_left, wide_right);
  auto& infinite = Operations::Subtract::create_synthetic(
      domain, infinity, negative_infinity);
  auto& unordered = Operations::Subtract::create_synthetic(domain, nan, one);

  EXPECT(narrow.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(narrow, source));
  EXPECT(link_operation(wide, source));
  EXPECT(link_operation(infinite, source));
  EXPECT(link_operation(unordered, source));

  auto narrow_value = selected(narrow.fold());
  auto wide_value = selected(wide.fold());
  auto infinite_value = selected(infinite.fold());
  auto unordered_value = selected(unordered.fold());
  auto narrow_number = narrow_value ? get_real(*narrow_value) : Option<R64>();
  auto wide_number = wide_value ? get_real(*wide_value) : Option<R64>();
  auto infinite_number =
      infinite_value ? get_real(*infinite_value) : Option<R64>();
  auto unordered_number =
      unordered_value ? get_real(*unordered_value) : Option<R64>();

  ASSERT(narrow_value && wide_value && infinite_value && unordered_value);
  EXPECT(narrow_number && *narrow_number == R64(R32(4.4) - R32(1.1)));
  EXPECT(&narrow_value->get_type() == &r32);
  EXPECT(wide_number && *wide_number == R64(-6.5));
  EXPECT(infinite_number && __builtin_isinf(*infinite_number));
  EXPECT(unordered_number && __builtin_isnan(*unordered_number));
}

PERIMORTEM_UNIT_TEST(LibrarySubtract, stable_folding) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 selected_type;
  auto& two = Constants::Unsigned::create_synthetic(domain, selected_type, 2);
  auto& twelve =
      Constants::Unsigned::create_synthetic(domain, selected_type, 12);
  auto& child = Operations::Multiply::create_synthetic(domain, two, two);
  auto& subtract =
      Operations::Subtract::create_synthetic(domain, twelve, child);

  EXPECT(subtract.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(subtract, source));
  EXPECT(link_operation(subtract, source));
  EXPECT(&subtract.get_type() == &selected_type);

  auto first = selected(subtract.fold());
  auto second = selected(subtract.fold());
  auto child_result = selected(child.fold());
  auto value = first ? get_unsigned(*first) : Option<U64>();

  ASSERT(first && second && child_result);
  EXPECT(&*first == &*second);
  EXPECT(first->is_identity<Constants::Unsigned>());
  EXPECT(&first->get_type() == &selected_type);
  EXPECT(value && *value == 8);
}
