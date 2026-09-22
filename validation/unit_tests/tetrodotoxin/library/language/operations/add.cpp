// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/add.hpp"

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

static Harness LibraryAdd = {
  .name = "Tetrodotoxin::Library::Language::Operations::Add"_view,
};

class AddExpression : public Expression {
 public:
  AddExpression(View::Bytes name, const Abstract& type)
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

static auto link_operation(Operation& operation, const Abstract& context)
    -> Bool {
  Allocator::Arena transaction;
  Tetrodotoxin::Source::Lexical::Errors errors;
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(transaction, {}, "<operation>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Tetrodotoxin::Source::Lexical::Cursor cursor(tokenizer, errors, associations);
  return operation.link(cursor, context);
}

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
      [](const Constants::Unsigned& value) -> Option<U64> {
        return value.get_value();
      },
      [](const Abstract&) -> Option<U64> { return {}; });
}

static auto get_signed(
    const Tetrodotoxin::Library::Language::Constant& expression)
    -> Option<S64> {
  return expression.visit<Constants::Signed>(
      [](const Constants::Signed& value) -> Option<S64> {
        return value.get_value();
      },
      [](const Abstract&) -> Option<S64> { return {}; });
}

static auto get_real(
    const Tetrodotoxin::Library::Language::Constant& expression)
    -> Option<R64> {
  return expression.visit<Constants::Real>(
      [](const Constants::Real& value) -> Option<R64> {
        return value.get_value();
      },
      [](const Abstract&) -> Option<R64> { return {}; });
}

PERIMORTEM_UNIT_TEST(LibraryAdd, type_selection) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 u8;
  Types::U16 u16;
  Types::S8 s8;
  Types::R32 r32;
  AddExpression unsigned_left("unsigned left"_view, u8);
  AddExpression unsigned_right("unsigned right"_view, u8);
  AddExpression other_width("other width"_view, u16);
  AddExpression signed_left("signed left"_view, s8);
  AddExpression signed_right("signed right"_view, s8);
  AddExpression real_left("real left"_view, r32);
  AddExpression real_right("real right"_view, r32);
  auto& unsigned_add =
      Operations::Add::create_synthetic(domain, unsigned_left, unsigned_right);
  auto& signed_add =
      Operations::Add::create_synthetic(domain, signed_left, signed_right);
  auto& real_add =
      Operations::Add::create_synthetic(domain, real_left, real_right);
  auto& mismatched =
      Operations::Add::create_synthetic(domain, unsigned_left, other_width);

  EXPECT(link_operation(unsigned_add, source));
  EXPECT(link_operation(signed_add, source));
  EXPECT(link_operation(real_add, source));
  EXPECT_NOT(link_operation(mismatched, source));
  EXPECT(&unsigned_add.get_type() == &u8);
  EXPECT(&signed_add.get_type() == &s8);
  EXPECT(&real_add.get_type() == &r32);
  EXPECT_NOT(selected(unsigned_add.fold()));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, integer_overflow) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 u8;
  Types::U64 u64;
  Types::S8 s8;
  auto& unsigned_max = Constants::Unsigned::create_synthetic(domain, u8, 255);
  auto& unsigned_one = Constants::Unsigned::create_synthetic(domain, u8, 1);
  auto& host_max = Constants::Unsigned::create_synthetic(domain, u64, U64(-1));
  auto& host_one = Constants::Unsigned::create_synthetic(domain, u64, 1);
  auto& signed_max = Constants::Signed::create_synthetic(domain, s8, 127);
  auto& signed_min = Constants::Signed::create_synthetic(domain, s8, -128);
  auto& signed_one = Constants::Signed::create_synthetic(domain, s8, 1);
  auto& signed_negative_one =
      Constants::Signed::create_synthetic(domain, s8, -1);
  auto& width_overflow =
      Operations::Add::create_synthetic(domain, unsigned_max, unsigned_one);
  auto& host_overflow =
      Operations::Add::create_synthetic(domain, host_max, host_one);
  auto& signed_overflow =
      Operations::Add::create_synthetic(domain, signed_max, signed_one);
  auto& signed_underflow = Operations::Add::create_synthetic(
      domain, signed_min, signed_negative_one);

  ASSERT(link_operation(width_overflow, source));
  ASSERT(link_operation(host_overflow, source));
  ASSERT(link_operation(signed_overflow, source));
  ASSERT(link_operation(signed_underflow, source));
  EXPECT(reports(
      width_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      width_overflow));
  EXPECT(reports(
      host_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      host_overflow));
  EXPECT(reports(
      signed_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      signed_overflow));
  EXPECT(reports(
      signed_underflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      signed_underflow));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, result_type) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 unsigned_type;
  Types::S8 signed_type;
  auto& unsigned_left =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 12);
  auto& unsigned_right =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 30);
  auto& signed_left =
      Constants::Signed::create_synthetic(domain, signed_type, -5);
  auto& signed_right =
      Constants::Signed::create_synthetic(domain, signed_type, 2);
  auto& unsigned_add =
      Operations::Add::create_synthetic(domain, unsigned_left, unsigned_right);
  auto& signed_add =
      Operations::Add::create_synthetic(domain, signed_left, signed_right);

  ASSERT(link_operation(unsigned_add, source));
  ASSERT(link_operation(signed_add, source));
  auto unsigned_result = selected(unsigned_add.fold());
  auto signed_result = selected(signed_add.fold());
  ASSERT(unsigned_result && signed_result);
  EXPECT(&unsigned_result->get_type() == &unsigned_type);
  EXPECT(&signed_result->get_type() == &signed_type);
  EXPECT(get_unsigned(*unsigned_result) == Option<U64>(42));
  EXPECT(get_signed(*signed_result) == Option<S64>(-3));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, ieee_real_domains) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::R32 r32;
  Types::R64 r64;
  auto& narrow_left = Constants::Real::create_synthetic(domain, r32, R64(1.1));
  auto& narrow_right = Constants::Real::create_synthetic(domain, r32, R64(2.2));
  auto& infinity =
      Constants::Real::create_synthetic(domain, r64, __builtin_inf());
  auto& one = Constants::Real::create_synthetic(domain, r64, R64(1));
  auto& nan = Constants::Real::create_synthetic(domain, r64, __builtin_nan(""));
  auto& narrow =
      Operations::Add::create_synthetic(domain, narrow_left, narrow_right);
  auto& infinite = Operations::Add::create_synthetic(domain, infinity, one);
  auto& unordered = Operations::Add::create_synthetic(domain, nan, one);

  ASSERT(link_operation(narrow, source));
  ASSERT(link_operation(infinite, source));
  ASSERT(link_operation(unordered, source));
  auto narrow_result = selected(narrow.fold());
  auto infinite_result = selected(infinite.fold());
  auto unordered_result = selected(unordered.fold());
  ASSERT(narrow_result && infinite_result && unordered_result);
  auto narrow_value = get_real(*narrow_result);
  auto infinite_value = get_real(*infinite_result);
  auto unordered_value = get_real(*unordered_result);
  EXPECT(narrow_value && *narrow_value == R64(R32(1.1) + R32(2.2)));
  EXPECT(&narrow_result->get_type() == &r32);
  EXPECT(infinite_value && __builtin_isinf(*infinite_value));
  EXPECT(unordered_value && __builtin_isnan(*unordered_value));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, stable_folding) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 type;
  auto& one = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& two = Constants::Unsigned::create_synthetic(domain, type, 2);
  auto& three = Constants::Unsigned::create_synthetic(domain, type, 3);
  auto& child = Operations::Add::create_synthetic(domain, one, two);
  auto& root = Operations::Add::create_synthetic(domain, child, three);

  ASSERT(link_operation(root, source));
  auto first = selected(root.fold());
  auto second = selected(root.fold());
  auto child_result = selected(child.fold());
  ASSERT(first && second && child_result);
  EXPECT(&*first == &*second);
  EXPECT(get_unsigned(*first) == Option<U64>(6));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, child_error_origin) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 type;
  auto& maximum = Constants::Unsigned::create_synthetic(domain, type, 255);
  auto& one = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& child = Operations::Add::create_synthetic(domain, maximum, one);
  auto& root = Operations::Add::create_synthetic(domain, child, one);

  ASSERT(link_operation(root, source));
  EXPECT(
      reports(root.fold(), Expression::Error::Type::ArithmeticOverflow, child));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, operand_error_origin) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 type;
  auto& wrong = Constants::Bytes::create_synthetic(domain, type, {});
  auto& valid = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& add = Operations::Add::create_synthetic(domain, wrong, valid);

  ASSERT(link_operation(add, source));
  EXPECT(reports(add.fold(), Expression::Error::Type::InvalidConstant, wrong));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, invalid_domains) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 u8;
  Types::S8 s8;
  Types::R32 r32;
  Types::Fixed bytes_type(
      "Fixed[U8,1]"_view, resolve_library_unsigned(source, "U8"_view), 1);
  auto& truth =
      Constants::True::create_synthetic(domain, resolve_library_flag(source));
  auto& bytes = Constants::Bytes::create_synthetic(domain, bytes_type, {});
  auto& unsigned_value = Constants::Unsigned::create_synthetic(domain, u8, 1);
  auto& signed_value = Constants::Signed::create_synthetic(domain, s8, 1);
  auto& real_value = Constants::Real::create_synthetic(domain, r32, 1.0);
  auto& bool_add = Operations::Add::create_synthetic(domain, truth, truth);
  auto& bytes_add = Operations::Add::create_synthetic(domain, bytes, bytes);
  auto& mixed_integer =
      Operations::Add::create_synthetic(domain, unsigned_value, signed_value);
  auto& mixed_domain =
      Operations::Add::create_synthetic(domain, unsigned_value, real_value);

  EXPECT_NOT(link_operation(bool_add, source));
  EXPECT_NOT(link_operation(bytes_add, source));
  EXPECT_NOT(link_operation(mixed_integer, source));
  EXPECT_NOT(link_operation(mixed_domain, source));
}
