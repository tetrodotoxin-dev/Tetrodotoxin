// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/negate.hpp"

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
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/r32.hpp"
#include "tetrodotoxin/library/language/types/r64.hpp"
#include "tetrodotoxin/library/language/types/s64.hpp"
#include "tetrodotoxin/library/language/types/s8.hpp"
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

static Harness LibraryNegate = {
  .name = "Tetrodotoxin::Library::Language::Operations::Negate"_view,
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

class NegateExpression : public Expression {
 public:
  NegateExpression(View::Bytes name, const Abstract& type)
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

PERIMORTEM_UNIT_TEST(LibraryNegate, type_selection) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::S8 s8;
  Types::R32 r32;
  Types::U8 u8;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[U8,1]"_view, resolve_library_unsigned(source, "U8"_view), 1);
  NegateExpression signed_value("signed"_view, s8);
  NegateExpression real_value("real"_view, r32);
  NegateExpression unsigned_value("unsigned"_view, u8);
  NegateExpression unresolved("unresolved"_view, Unknown::get_unknown());
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& signed_negate =
      Operations::Negate::create_synthetic(domain, signed_value);
  auto& real_negate = Operations::Negate::create_synthetic(domain, real_value);
  auto& unsigned_negate =
      Operations::Negate::create_synthetic(domain, unsigned_value);
  auto& flag_negate = Operations::Negate::create_synthetic(domain, truth);
  auto& bytes_negate = Operations::Negate::create_synthetic(domain, bytes);
  auto& invalid_negate =
      Operations::Negate::create_synthetic(domain, unresolved);

  EXPECT(signed_negate.get_type().resolve().is<Unknown>());
  EXPECT_NOT(signed_negate.get_anchor());
  EXPECT(link_operation(signed_negate, source));
  EXPECT(link_operation(real_negate, source));
  EXPECT(!link_operation(unsigned_negate, source));
  EXPECT(!link_operation(flag_negate, source));
  EXPECT(!link_operation(bytes_negate, source));
  EXPECT(!link_operation(invalid_negate, source));

  auto signed_result = selected(signed_negate.fold());
  auto real_result = selected(real_negate.fold());

  EXPECT_NOT(signed_result);
  EXPECT_NOT(real_result);
  EXPECT(&signed_negate.get_type() == &s8);
  EXPECT(&real_negate.get_type() == &r32);
  EXPECT(unsigned_negate.get_type().resolve().is<Unknown>());
  EXPECT(flag_negate.get_type().resolve().is<Unknown>());
  EXPECT(bytes_negate.get_type().resolve().is<Unknown>());
  EXPECT(invalid_negate.get_type().resolve().is<Unknown>());
}

PERIMORTEM_UNIT_TEST(LibraryNegate, signed_widths) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::S8 s8;
  Types::S64 s64;
  auto& positive = Constants::Signed::create_synthetic(domain, s8, 127);
  auto& negative = Constants::Signed::create_synthetic(domain, s8, -127);
  auto& zero = Constants::Signed::create_synthetic(domain, s8, 0);
  auto& minimum = Constants::Signed::create_synthetic(domain, s8, -128);
  auto& wide_minimum = Constants::Signed::create_synthetic(
      domain, s64, S64(-9223372036854775807) - 1);
  auto& positive_negate =
      Operations::Negate::create_synthetic(domain, positive);
  auto& negative_negate =
      Operations::Negate::create_synthetic(domain, negative);
  auto& zero_negate = Operations::Negate::create_synthetic(domain, zero);
  auto& minimum_negate = Operations::Negate::create_synthetic(domain, minimum);
  auto& wide_minimum_negate =
      Operations::Negate::create_synthetic(domain, wide_minimum);

  EXPECT(positive_negate.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(positive_negate, source));
  EXPECT(link_operation(negative_negate, source));
  EXPECT(link_operation(zero_negate, source));
  EXPECT(link_operation(minimum_negate, source));
  EXPECT(link_operation(wide_minimum_negate, source));

  auto positive_result = selected(positive_negate.fold());
  auto negative_result = selected(negative_negate.fold());
  auto zero_result = selected(zero_negate.fold());
  auto minimum_result = minimum_negate.fold();
  auto wide_minimum_result = wide_minimum_negate.fold();
  auto positive_value =
      positive_result ? get_signed(*positive_result) : Option<S64>();
  auto negative_value =
      negative_result ? get_signed(*negative_result) : Option<S64>();
  auto zero_value = zero_result ? get_signed(*zero_result) : Option<S64>();

  ASSERT(positive_result && negative_result && zero_result);
  EXPECT(positive_value && *positive_value == -127);
  EXPECT(negative_value && *negative_value == 127);
  EXPECT(zero_value && *zero_value == 0);
  EXPECT(&positive_result->get_type() == &s8);
  EXPECT(reports(
      minimum_result, Expression::Error::Type::ArithmeticOverflow,
      minimum_negate));
  EXPECT(reports(
      wide_minimum_result, Expression::Error::Type::ArithmeticOverflow,
      wide_minimum_negate));
}

PERIMORTEM_UNIT_TEST(LibraryNegate, ieee_real_domains) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::R32 r32;
  Types::R64 r64;
  auto& finite_32 = Constants::Real::create_synthetic(domain, r32, 3.25);
  auto& finite_64 = Constants::Real::create_synthetic(domain, r64, -9.5);
  auto& infinity =
      Constants::Real::create_synthetic(domain, r64, __builtin_inf());
  auto& nan = Constants::Real::create_synthetic(domain, r64, __builtin_nan(""));
  auto& positive_zero = Constants::Real::create_synthetic(domain, r32, 0.0);
  auto& negative_zero = Constants::Real::create_synthetic(domain, r64, -0.0);
  auto& finite_32_negate =
      Operations::Negate::create_synthetic(domain, finite_32);
  auto& finite_64_negate =
      Operations::Negate::create_synthetic(domain, finite_64);
  auto& infinity_negate =
      Operations::Negate::create_synthetic(domain, infinity);
  auto& nan_negate = Operations::Negate::create_synthetic(domain, nan);
  auto& positive_zero_negate =
      Operations::Negate::create_synthetic(domain, positive_zero);
  auto& negative_zero_negate =
      Operations::Negate::create_synthetic(domain, negative_zero);

  EXPECT(finite_32_negate.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(finite_32_negate, source));
  EXPECT(link_operation(finite_64_negate, source));
  EXPECT(link_operation(infinity_negate, source));
  EXPECT(link_operation(nan_negate, source));
  EXPECT(link_operation(positive_zero_negate, source));
  EXPECT(link_operation(negative_zero_negate, source));

  auto finite_32_result = selected(finite_32_negate.fold());
  auto finite_64_result = selected(finite_64_negate.fold());
  auto infinity_result = selected(infinity_negate.fold());
  auto nan_result = selected(nan_negate.fold());
  auto positive_zero_result = selected(positive_zero_negate.fold());
  auto negative_zero_result = selected(negative_zero_negate.fold());
  auto finite_32_value =
      finite_32_result ? get_real(*finite_32_result) : Option<R64>();
  auto finite_64_value =
      finite_64_result ? get_real(*finite_64_result) : Option<R64>();
  auto infinity_value =
      infinity_result ? get_real(*infinity_result) : Option<R64>();
  auto nan_value = nan_result ? get_real(*nan_result) : Option<R64>();
  auto positive_zero_value =
      positive_zero_result ? get_real(*positive_zero_result) : Option<R64>();
  auto negative_zero_value =
      negative_zero_result ? get_real(*negative_zero_result) : Option<R64>();

  ASSERT(
      finite_32_value && finite_64_value && infinity_value && nan_value &&
      positive_zero_value && negative_zero_value);
  EXPECT(*finite_32_value == -3.25);
  EXPECT(*finite_64_value == 9.5);
  EXPECT(__builtin_isinf(*infinity_value) && *infinity_value < 0.0);
  EXPECT(__builtin_isnan(*nan_value));
  EXPECT(
      *positive_zero_value == 0.0 && __builtin_signbit(*positive_zero_value));
  EXPECT(
      *negative_zero_value == 0.0 && !__builtin_signbit(*negative_zero_value));
  EXPECT(&finite_32_result->get_type() == &r32);
  EXPECT(&finite_64_result->get_type() == &r64);
}

PERIMORTEM_UNIT_TEST(LibraryNegate, recursive_folding) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::S8 s8;
  auto& one = Constants::Signed::create_synthetic(domain, s8, 1);
  auto& minimum = Constants::Signed::create_synthetic(domain, s8, -128);
  auto& child = Operations::Negate::create_synthetic(domain, one);
  auto& parent = Operations::Negate::create_synthetic(domain, child);
  auto& failing_child = Operations::Negate::create_synthetic(domain, minimum);
  auto& failing_parent =
      Operations::Negate::create_synthetic(domain, failing_child);

  EXPECT(parent.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(parent, source));
  EXPECT(link_operation(failing_parent, source));

  auto parent_result = selected(parent.fold());
  auto repeated_result = selected(parent.fold());
  auto failing_result = failing_parent.fold();
  auto parent_value =
      parent_result ? get_signed(*parent_result) : Option<S64>();

  ASSERT(parent_result && repeated_result && parent_value);
  EXPECT(*parent_value == 1);
  EXPECT(&*parent_result == &*repeated_result);
  EXPECT(reports(
      failing_result, Expression::Error::Type::ArithmeticOverflow,
      failing_child));
}
