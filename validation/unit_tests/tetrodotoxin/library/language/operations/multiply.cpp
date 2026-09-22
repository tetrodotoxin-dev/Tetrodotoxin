// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/multiply.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
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
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness LibraryMultiply = {
  .name = "Tetrodotoxin::Library::Language::Operations::Multiply"_view,
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

class MultiplyExpression : public Expression {
 public:
  MultiplyExpression(View::Bytes name, const Abstract& type)
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

class MultiplyFoldInput : public Operation {
 public:
  MultiplyFoldInput(
      Allocator::Arena& domain,
      Model::Pack& input,
      Tetrodotoxin::Library::Language::Constant& result,
      const Model::Type& type)
      : Operation(
            domain,
            Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 1>{{input}},
            {}),
        result(result),
        type(type) {}

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
    return result;
  }

  auto select_type(const Abstract&) const
      -> Option<const Model::Type&> override {
    return type;
  }

 private:
  Tetrodotoxin::Library::Language::Constant& result;
  const Model::Type& type;
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

PERIMORTEM_UNIT_TEST(LibraryMultiply, type_selection) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 u8;
  Types::U16 u16;
  Types::U64 u64;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[U8,1]"_view, resolve_library_unsigned(source, "U8"_view), 1);
  MultiplyExpression left("left"_view, u8);
  MultiplyExpression same("same"_view, u8);
  MultiplyExpression other("other"_view, u16);
  MultiplyExpression unresolved("unresolved"_view, Unknown::get_unknown());
  auto& wide_constant = Constants::Unsigned::create_synthetic(domain, u64, 12);
  auto& other_constant = Constants::Unsigned::create_synthetic(domain, u16, 12);
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& exact = Operations::Multiply::create_synthetic(domain, left, same);
  auto& mixed_left =
      Operations::Multiply::create_synthetic(domain, wide_constant, left);
  auto& mismatch = Operations::Multiply::create_synthetic(domain, left, other);
  auto& mixed_constants = Operations::Multiply::create_synthetic(
      domain, wide_constant, other_constant);
  auto& flags = Operations::Multiply::create_synthetic(domain, truth, truth);
  auto& byte_values =
      Operations::Multiply::create_synthetic(domain, bytes, bytes);
  auto& invalid =
      Operations::Multiply::create_synthetic(domain, unresolved, same);

  EXPECT(exact.get_type().resolve().is<Unknown>());
  EXPECT_NOT(exact.get_anchor());
  EXPECT(link_operation(exact, source));
  EXPECT(!link_operation(mixed_left, source));
  EXPECT(!link_operation(mismatch, source));
  EXPECT(!link_operation(mixed_constants, source));
  EXPECT(!link_operation(flags, source));
  EXPECT(!link_operation(byte_values, source));
  EXPECT(!link_operation(invalid, source));

  auto exact_result = selected(exact.fold());

  EXPECT(&exact.get_type() == &u8);
  EXPECT(mixed_left.get_type().resolve().is<Unknown>());
  EXPECT_NOT(exact_result);
  EXPECT(mismatch.get_type().resolve().is<Unknown>());
  EXPECT(mixed_constants.get_type().resolve().is<Unknown>());
  EXPECT(flags.get_type().resolve().is<Unknown>());
  EXPECT(byte_values.get_type().resolve().is<Unknown>());
  EXPECT(invalid.get_type().resolve().is<Unknown>());
}

PERIMORTEM_UNIT_TEST(LibraryMultiply, integer_widths) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 unsigned_type;
  Types::S8 signed_type;
  auto& fifteen =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 15);
  auto& seventeen =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 17);
  auto& sixteen =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 16);
  auto& zero = Constants::Unsigned::create_synthetic(domain, unsigned_type, 0);
  auto& negative_twelve =
      Constants::Signed::create_synthetic(domain, signed_type, -12);
  auto& negative_ten =
      Constants::Signed::create_synthetic(domain, signed_type, -10);
  auto& minimum =
      Constants::Signed::create_synthetic(domain, signed_type, -128);
  auto& negative_one =
      Constants::Signed::create_synthetic(domain, signed_type, -1);
  auto& one = Constants::Signed::create_synthetic(domain, signed_type, 1);
  auto& unsigned_success =
      Operations::Multiply::create_synthetic(domain, fifteen, seventeen);
  auto& unsigned_overflow =
      Operations::Multiply::create_synthetic(domain, sixteen, sixteen);
  auto& zero_product =
      Operations::Multiply::create_synthetic(domain, zero, seventeen);
  auto& signed_success = Operations::Multiply::create_synthetic(
      domain, negative_twelve, negative_ten);
  auto& endpoint = Operations::Multiply::create_synthetic(domain, minimum, one);
  auto& signed_overflow =
      Operations::Multiply::create_synthetic(domain, minimum, negative_one);

  EXPECT(unsigned_success.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(unsigned_success, source));
  EXPECT(link_operation(unsigned_overflow, source));
  EXPECT(link_operation(zero_product, source));
  EXPECT(link_operation(signed_success, source));
  EXPECT(link_operation(endpoint, source));
  EXPECT(link_operation(signed_overflow, source));

  auto unsigned_value = selected(unsigned_success.fold());
  auto zero_value = selected(zero_product.fold());
  auto signed_value = selected(signed_success.fold());
  auto endpoint_value = selected(endpoint.fold());
  auto unsigned_number =
      unsigned_value ? get_unsigned(*unsigned_value) : Option<U64>();
  auto zero_number = zero_value ? get_unsigned(*zero_value) : Option<U64>();
  auto signed_number = signed_value ? get_signed(*signed_value) : Option<S64>();
  auto endpoint_number =
      endpoint_value ? get_signed(*endpoint_value) : Option<S64>();

  ASSERT(unsigned_value && zero_value && signed_value && endpoint_value);
  EXPECT(&unsigned_value->get_type() == &unsigned_type);
  EXPECT(unsigned_number && *unsigned_number == 255);
  EXPECT(zero_number && *zero_number == 0);
  EXPECT(&signed_value->get_type() == &signed_type);
  EXPECT(signed_number && *signed_number == 120);
  EXPECT(endpoint_number && *endpoint_number == -128);
  EXPECT(reports(
      unsigned_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      unsigned_overflow));
  EXPECT(reports(
      signed_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      signed_overflow));
}

PERIMORTEM_UNIT_TEST(LibraryMultiply, ieee_real_domains) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::R32 r32;
  Types::R64 r64;
  auto& narrow_left = Constants::Real::create_synthetic(domain, r32, R64(1.1));
  auto& narrow_right = Constants::Real::create_synthetic(domain, r32, R64(3.0));
  auto& wide_left = Constants::Real::create_synthetic(domain, r64, R64(-2.5));
  auto& wide_right = Constants::Real::create_synthetic(domain, r64, R64(4.0));
  auto& infinity =
      Constants::Real::create_synthetic(domain, r64, __builtin_inf());
  auto& nan = Constants::Real::create_synthetic(domain, r64, __builtin_nan(""));
  auto& one = Constants::Real::create_synthetic(domain, r64, R64(1.0));
  auto& narrow =
      Operations::Multiply::create_synthetic(domain, narrow_left, narrow_right);
  auto& wide =
      Operations::Multiply::create_synthetic(domain, wide_left, wide_right);
  auto& infinite =
      Operations::Multiply::create_synthetic(domain, infinity, one);
  auto& unordered = Operations::Multiply::create_synthetic(domain, nan, one);

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
  EXPECT(&narrow_value->get_type() == &r32);
  EXPECT(narrow_number && *narrow_number == R64(R32(1.1) * R32(3.0)));
  EXPECT(&wide_value->get_type() == &r64);
  EXPECT(wide_number && *wide_number == R64(-10.0));
  EXPECT(infinite_number && __builtin_isinf(*infinite_number));
  EXPECT(unordered_number && __builtin_isnan(*unordered_number));
}

PERIMORTEM_UNIT_TEST(LibraryMultiply, stable_folding) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 selected_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, selected_type, 1);
  auto& folded =
      Constants::Unsigned::create_synthetic(domain, selected_type, 4);
  auto& factor =
      Constants::Unsigned::create_synthetic(domain, selected_type, 3);
  MultiplyFoldInput child(domain, input, folded, selected_type);
  auto& multiply =
      Operations::Multiply::create_synthetic(domain, factor, child);

  EXPECT(multiply.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(multiply, source));
  EXPECT(link_operation(multiply, source));
  EXPECT(&multiply.get_type() == &selected_type);

  auto first = selected(multiply.fold());
  auto second = selected(multiply.fold());
  auto value = first ? get_unsigned(*first) : Option<U64>();

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(first->is_identity<Constants::Unsigned>());
  EXPECT(&first->get_type() == &selected_type);
  EXPECT(value && *value == 12);
  EXPECT(child.get_evaluations() == 1);
}
