// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/slice.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/operation.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/s64.hpp"
#include "tetrodotoxin/library/language/types/u64.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Validation;
using Tetrodotoxin::Library::Language::Access::Slice;

static Harness LibrarySlice = {
  .name = "Tetrodotoxin::Library::Language::Access::Slice"_view,
};

static auto link_expression(
    Allocator::Arena& domain,
    Expression& expression,
    const Abstract& context) -> Bool {
  Tetrodotoxin::Source::Lexical::Errors errors;
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(domain, {}, "slice-expression.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Tetrodotoxin::Source::Lexical::Cursor cursor(tokenizer, errors, associations);
  return expression.link(cursor, context);
}

static auto create_slice(
    Allocator::Arena& domain,
    Model::Pack& receiver,
    Model::Pack& index) -> Slice& {
  return Slice::create_authored(
      domain, receiver, index,
      Tetrodotoxin::Source::Lexical::Anchor::create(Tetrodotoxin::Source::Lexical::Span()));
}

static auto create_slice(
    Allocator::Arena& domain,
    Model::Pack& receiver,
    Model::Pack& start,
    Model::Pack& count) -> Slice& {
  return Slice::create_authored(
      domain, receiver, start, count,
      Tetrodotoxin::Source::Lexical::Anchor::create(Tetrodotoxin::Source::Lexical::Span()));
}

class ValueExpression : public Expression {
 public:
  ValueExpression(View::Bytes name, const Tetrodotoxin::Source::Type& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto get_type() const -> const Tetrodotoxin::Source::Type& override { return type; }

 private:
  View::Bytes name;
  const Tetrodotoxin::Source::Type& type;
};

class ValueConstant : public Tetrodotoxin::Library::Language::Constant {
 public:
  explicit ValueConstant(const Model::Type& type)
      : Tetrodotoxin::Library::Language::Constant({}), type(type) {}

  constexpr auto get_type() const -> const Model::Type& override {
    return type;
  }
  constexpr auto equals(const Tetrodotoxin::Library::Language::Constant& rhs)
      const -> Bool override {
    return has_same_type(rhs);
  }

 private:
  const Model::Type& type;
};

class ValueFoldOperation : public Operation {
 public:
  ValueFoldOperation(
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

  auto get_name() const -> View::Bytes override { return "Fold size"_view; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }

 protected:
  auto evaluate_constants(Allocator::Arena&) -> Result<
      Option<Tetrodotoxin::Library::Language::Constant&>,
      Expression::Error> override {
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
};

static auto is_dynamic(
    const Result<Option<Model::Pack&>, Expression::Error>& result) -> Bool {
  return result.visit(
      [](const Option<Model::Pack&>& selected) {
        return !selected ? True : False;
      },
      [](const Expression::Error&) { return False; });
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

static auto selected_pack(
    const Result<Option<Model::Pack&>, Expression::Error>& result)
    -> Option<Model::Pack&> {
  return result.visit(
      [](const Option<Model::Pack&>& folded) { return folded; },
      [](const Expression::Error&) -> Option<Model::Pack&> { return {}; });
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
    const Tetrodotoxin::Library::Language::Constant& constant) -> Option<U64> {
  return constant.visit<Constants::Unsigned>(
      [](const Constants::Unsigned& selected) -> Option<U64> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<U64> { return {}; });
}

static auto get_unsigned(const Model::Pack& pack, Count index) -> Option<U64> {
  auto entry = pack.get_layout().get_abstract(index);
  BAIL_IF(!entry);
  auto constant = entry->select<Constants::Unsigned>();
  return constant ? Option<U64>(constant->get_value()) : Option<U64>();
}

static auto get_signed(
    const Tetrodotoxin::Library::Language::Constant& constant) -> Option<S64> {
  return constant.visit<Constants::Signed>(
      [](const Constants::Signed& selected) -> Option<S64> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<S64> { return {}; });
}

static auto get_real(const Tetrodotoxin::Library::Language::Constant& constant)
    -> Option<R64> {
  return constant.visit<Constants::Real>(
      [](const Constants::Real& selected) -> Option<R64> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<R64> { return {}; });
}

static auto supplies_self(const Slice& value, Count size) -> Bool {
  const Layout& layout = value.get_layout();
  BAIL_IF(layout.get_size() != size);
  for (Count index = 0; index < size; index++) {
    auto source = layout.get_abstract(index);
    BAIL_IF(!source || &*source != &value);
  }
  return True;
}

PERIMORTEM_UNIT_TEST(LibrarySlice, receiver_type) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  const auto& element = resolve_library_unsigned(source, "U8"_view);
  Types::S64 integer;
  Types::Fixed fixed("Fixed[U8,1]"_view, element, 1);
  Types::View view("View[U8]"_view, element);
  Types::Access access("Access[U8]"_view, element);
  ValueExpression fixed_receiver("fixed"_view, fixed);
  ValueExpression view_receiver("view"_view, view);
  ValueExpression access_receiver("access"_view, access);
  ValueExpression index("index"_view, integer);
  auto& fixed_index = create_slice(domain, fixed_receiver, index);
  auto& view_index = create_slice(domain, view_receiver, index);
  auto& access_index = create_slice(domain, access_receiver, index);

  EXPECT(fixed_index.get_type().resolve().is<Unknown>());
  EXPECT(fixed_index.is<Expression>());
  EXPECT_NOT(fixed_index.is<Operation>());
  EXPECT(fixed_index.get_anchor());
  EXPECT(link_expression(domain, fixed_index, source));
  EXPECT(link_expression(domain, view_index, source));
  EXPECT(link_expression(domain, access_index, source));

  EXPECT(&fixed_index.get_type() == &element);
  EXPECT(&view_index.get_type() == &element);
  EXPECT(&access_index.get_type() == &element);
  EXPECT(is_dynamic(fixed_index.fold()));
  EXPECT(is_dynamic(view_index.fold()));
  EXPECT(is_dynamic(access_index.fold()));
}

PERIMORTEM_UNIT_TEST(LibrarySlice, range_pack_shape) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 element;
  Types::S64 integer;
  Types::U64 unsigned_integer;
  Types::Fixed fixed("Fixed[U8,8]"_view, element, 8);
  Types::View view("View[U8]"_view, element);
  Types::Access access("Access[U8]"_view, element);
  ValueExpression fixed_receiver("fixed"_view, fixed);
  ValueExpression view_receiver("view"_view, view);
  ValueExpression access_receiver("access"_view, access);
  ValueExpression start("start"_view, integer);
  ValueExpression dynamic_size("size"_view, integer);
  auto& fold_input =
      Constants::Unsigned::create_synthetic(domain, unsigned_integer, 1);
  auto& fixed_size =
      Constants::Unsigned::create_synthetic(domain, unsigned_integer, 4);
  auto& empty_size =
      Constants::Unsigned::create_synthetic(domain, unsigned_integer, 0);
  ValueFoldOperation size_operation(
      domain, fold_input, fixed_size, unsigned_integer);
  auto& fixed_dynamic =
      create_slice(domain, fixed_receiver, start, dynamic_size);
  auto& view_dynamic = create_slice(domain, view_receiver, start, dynamic_size);
  auto& access_dynamic =
      create_slice(domain, access_receiver, start, dynamic_size);
  auto& constant_size = create_slice(domain, fixed_receiver, start, fixed_size);
  auto& folded_size =
      create_slice(domain, fixed_receiver, start, size_operation);
  auto& view_size = create_slice(domain, view_receiver, start, fixed_size);
  auto& access_size = create_slice(domain, access_receiver, start, fixed_size);
  auto& single_size = create_slice(domain, fixed_receiver, start, fold_input);
  auto& empty = create_slice(domain, fixed_receiver, start, empty_size);

  EXPECT(fixed_dynamic.get_type().resolve().is<Unknown>());
  EXPECT(!link_expression(domain, fixed_dynamic, source));
  EXPECT(!link_expression(domain, view_dynamic, source));
  EXPECT(!link_expression(domain, access_dynamic, source));
  EXPECT(link_expression(domain, constant_size, source));
  EXPECT(link_expression(domain, folded_size, source));
  EXPECT(link_expression(domain, view_size, source));
  EXPECT(link_expression(domain, access_size, source));
  EXPECT(link_expression(domain, single_size, source));
  EXPECT(link_expression(domain, empty, source));

  auto folded_result = folded_size.fold();

  EXPECT(constant_size.get_type().is<Unknown>());
  EXPECT(folded_size.get_type().is<Unknown>());
  EXPECT(view_size.get_type().is<Unknown>());
  EXPECT(access_size.get_type().is<Unknown>());
  EXPECT(&single_size.get_type() == &element);
  EXPECT(empty.get_type().is<Unknown>());
  EXPECT(is_dynamic(folded_result));
  EXPECT(supplies_self(constant_size, 4));
  EXPECT(supplies_self(folded_size, 4));
  EXPECT(supplies_self(view_size, 4));
  EXPECT(supplies_self(access_size, 4));
  EXPECT(supplies_self(single_size, 1));
  EXPECT(supplies_self(empty, 0));
  Types::Fixed four_values("Fixed[U8,4]"_view, element, 4);
  EXPECT(constant_size.get_layout().fits(four_values.get_layout()));
  EXPECT(empty.get_layout().is_empty());
}

PERIMORTEM_UNIT_TEST(LibrarySlice, folded_selection) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  const auto& element = resolve_library_unsigned(source, "U8"_view);
  Types::U64 integer;
  Types::Fixed bytes_type("Fixed[U8,6]"_view, element, 6);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "abcdef"_view);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& one = Constants::Unsigned::create_synthetic(domain, integer, 1);
  auto& two = Constants::Unsigned::create_synthetic(domain, integer, 2);
  auto& four = Constants::Unsigned::create_synthetic(domain, integer, 4);
  auto& six = Constants::Unsigned::create_synthetic(domain, integer, 6);
  auto& index = create_slice(domain, bytes, one);
  auto& full = create_slice(domain, bytes, zero, six);
  auto& interior = create_slice(domain, bytes, one, four);
  auto& empty = create_slice(domain, bytes, two, zero);
  auto& terminal_empty = create_slice(domain, bytes, six, zero);

  EXPECT(index.get_type().resolve().is<Unknown>());
  EXPECT(link_expression(domain, index, source));
  EXPECT(link_expression(domain, full, source));
  EXPECT(link_expression(domain, interior, source));
  EXPECT(link_expression(domain, empty, source));
  EXPECT(link_expression(domain, terminal_empty, source));

  auto indexed = selected(index.fold());
  auto full_value = selected_pack(full.fold());
  auto interior_value = selected_pack(interior.fold());
  auto empty_value = selected_pack(empty.fold());
  auto terminal_value = selected_pack(terminal_empty.fold());
  auto indexed_byte = indexed ? get_unsigned(*indexed) : Option<U64>();

  ASSERT(indexed);
  EXPECT(indexed->is_identity<Constants::Unsigned>());
  EXPECT(indexed_byte && *indexed_byte == U64('b'));
  EXPECT(&indexed->get_type() == &element);
  ASSERT(full_value);
  ASSERT(interior_value);
  ASSERT(empty_value);
  ASSERT(terminal_value);
  EXPECT_EQ(full_value->get_layout().get_size(), 6);
  EXPECT_EQ(interior_value->get_layout().get_size(), 4);
  EXPECT_EQ(empty_value->get_layout().get_size(), 0);
  EXPECT_EQ(terminal_value->get_layout().get_size(), 0);
  auto full_first = get_unsigned(*full_value, 0);
  auto full_last = get_unsigned(*full_value, 5);
  auto interior_first = get_unsigned(*interior_value, 0);
  auto interior_last = get_unsigned(*interior_value, 3);
  ASSERT(full_first && full_last && interior_first && interior_last);
  EXPECT_EQ(*full_first, U64('a'));
  EXPECT_EQ(*full_last, U64('f'));
  EXPECT_EQ(*interior_first, U64('b'));
  EXPECT_EQ(*interior_last, U64('e'));
  EXPECT(supplies_self(full, 6));
  EXPECT(supplies_self(interior, 4));
  EXPECT(supplies_self(empty, 0));
  EXPECT(supplies_self(terminal_empty, 0));
}

PERIMORTEM_UNIT_TEST(LibrarySlice, partial_folding) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 element;
  Types::U64 integer;
  Types::Fixed fixed("Fixed[U8,4]"_view, element, 4);
  ValueExpression dynamic_receiver("receiver"_view, fixed);
  ValueExpression dynamic_index("index"_view, integer);
  ValueExpression dynamic_start("start"_view, integer);
  ValueExpression dynamic_size("size"_view, integer);
  auto& bytes = Constants::Bytes::create_synthetic(domain, fixed, "abcd"_view);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& two = Constants::Unsigned::create_synthetic(domain, integer, 2);
  auto& receiver_partial = create_slice(domain, dynamic_receiver, zero);
  auto& index_partial = create_slice(domain, bytes, dynamic_index);
  auto& start_partial = create_slice(domain, bytes, dynamic_start, two);
  auto& size_partial = create_slice(domain, bytes, zero, dynamic_size);

  EXPECT(receiver_partial.get_type().resolve().is<Unknown>());
  EXPECT(link_expression(domain, receiver_partial, source));
  EXPECT(link_expression(domain, index_partial, source));
  EXPECT(link_expression(domain, start_partial, source));
  EXPECT(!link_expression(domain, size_partial, source));

  EXPECT(is_dynamic(receiver_partial.fold()));
  EXPECT(is_dynamic(index_partial.fold()));
  EXPECT(is_dynamic(start_partial.fold()));
  EXPECT(&receiver_partial.get_type() == &element);
  EXPECT(&index_partial.get_type() == &element);
  EXPECT(start_partial.get_type().is<Unknown>());
  EXPECT(supplies_self(start_partial, 2));
  EXPECT(size_partial.get_type().is<Unknown>());
}

PERIMORTEM_UNIT_TEST(LibrarySlice, safe_bounds) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  const auto& element = resolve_library_unsigned(source, "U8"_view);
  Types::U64 integer;
  Types::S64 signed_integer;
  Types::Boolean flag_type;
  Types::Fixed fixed("Fixed[U8,3]"_view, element, 3);
  auto& bytes = Constants::Bytes::create_synthetic(domain, fixed, "abc"_view);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& two = Constants::Unsigned::create_synthetic(domain, integer, 2);
  auto& three = Constants::Unsigned::create_synthetic(domain, integer, 3);
  auto& four = Constants::Unsigned::create_synthetic(domain, integer, 4);
  auto& maximum =
      Constants::Unsigned::create_synthetic(domain, integer, U64(-1));
  auto& negative =
      Constants::Signed::create_synthetic(domain, signed_integer, -1);
  auto& flag = Constants::True::create_synthetic(domain, flag_type);
  auto& invalid_receiver = create_slice(domain, flag, zero);
  auto& invalid_operand = create_slice(domain, bytes, flag);
  auto& invalid_count = create_slice(domain, bytes, zero, flag);
  auto& negative_index = create_slice(domain, bytes, negative);
  auto& maximum_index = create_slice(domain, bytes, maximum);
  auto& maximum_range = create_slice(domain, bytes, zero, maximum);
  auto& negative_start = create_slice(domain, bytes, negative, two);
  auto& negative_size = create_slice(domain, bytes, zero, negative);
  auto& index_bounds = create_slice(domain, bytes, three);
  ValueFoldOperation nested_index(domain, zero, three, integer);
  auto& nested_index_bounds = create_slice(domain, bytes, nested_index);
  auto& start_bounds = create_slice(domain, bytes, four, zero);
  auto& size_bounds = create_slice(domain, bytes, two, two);

  EXPECT(invalid_receiver.get_type().resolve().is<Unknown>());
  EXPECT(!link_expression(domain, invalid_receiver, source));
  EXPECT(!link_expression(domain, invalid_operand, source));
  EXPECT(!link_expression(domain, invalid_count, source));
  EXPECT(link_expression(domain, negative_index, source));
  EXPECT(link_expression(domain, maximum_index, source));
  EXPECT(link_expression(domain, maximum_range, source));
  EXPECT(link_expression(domain, negative_start, source));
  EXPECT(!link_expression(domain, negative_size, source));
  EXPECT(link_expression(domain, index_bounds, source));
  EXPECT(link_expression(domain, nested_index_bounds, source));
  EXPECT(link_expression(domain, start_bounds, source));
  EXPECT(link_expression(domain, size_bounds, source));

  EXPECT(is_dynamic(invalid_receiver.fold()));
  EXPECT(is_dynamic(invalid_operand.fold()));
  EXPECT(is_dynamic(invalid_count.fold()));
  auto negative_index_value = selected(negative_index.fold());
  auto maximum_index_value = selected(maximum_index.fold());
  auto index_bounds_value = selected(index_bounds.fold());
  auto nested_index_value = selected(nested_index_bounds.fold());
  auto negative_index_default = negative_index_value
                                    ? get_unsigned(*negative_index_value)
                                    : Option<U64>();
  auto maximum_index_default =
      maximum_index_value ? get_unsigned(*maximum_index_value) : Option<U64>();
  auto index_bounds_default =
      index_bounds_value ? get_unsigned(*index_bounds_value) : Option<U64>();
  auto nested_index_default =
      nested_index_value ? get_unsigned(*nested_index_value) : Option<U64>();
  ASSERT(
      negative_index_default && maximum_index_default && index_bounds_default &&
      nested_index_default);
  EXPECT(*negative_index_default == 0);
  EXPECT(*maximum_index_default == 0);
  EXPECT(*index_bounds_default == 0);
  EXPECT(*nested_index_default == 0);
  EXPECT(&negative_index_value->get_type() == &element);
  EXPECT(&maximum_index_value->get_type() == &element);
  EXPECT(&index_bounds_value->get_type() == &element);
  EXPECT(&nested_index_value->get_type() == &element);
  auto negative_range = selected_pack(negative_start.fold());
  EXPECT(is_dynamic(maximum_range.fold()));
  auto empty_range = selected_pack(start_bounds.fold());
  auto partial_range = selected_pack(size_bounds.fold());
  ASSERT(negative_range && empty_range && partial_range);
  EXPECT_EQ(negative_range->get_layout().get_size(), Count(2));
  EXPECT_EQ(empty_range->get_layout().get_size(), Count(0));
  EXPECT_EQ(partial_range->get_layout().get_size(), Count(2));
  EXPECT_EQ(get_unsigned(*negative_range, 0), Option<U64>(0));
  EXPECT_EQ(get_unsigned(*negative_range, 1), Option<U64>(0));
  EXPECT_EQ(get_unsigned(*partial_range, 0), Option<U64>(U64('c')));
  EXPECT_EQ(get_unsigned(*partial_range, 1), Option<U64>(0));
  EXPECT(supplies_self(negative_start, 2));
  EXPECT_EQ(maximum_range.get_layout().get_size(), Count(-1));
  EXPECT(supplies_self(start_bounds, 0));
  EXPECT(supplies_self(size_bounds, 2));
  EXPECT(&invalid_receiver.get_type() == &Unknown::get_unknown());
  EXPECT(&invalid_operand.get_type() == &Unknown::get_unknown());
  EXPECT(&invalid_count.get_type() == &Unknown::get_unknown());
}

PERIMORTEM_UNIT_TEST(LibrarySlice, scalar_defaults) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  const auto& boolean = resolve_library_flag(source);
  const auto& signed_integer = resolve_library_signed(source, "S64"_view);
  const auto& real = resolve_library_real(source, "R64"_view);
  const auto& index_type = resolve_library_unsigned(source, "U64"_view);
  Types::Fixed booleans("Fixed[Bool,1]"_view, boolean, 1);
  Types::Fixed signed_values("Fixed[S64,1]"_view, signed_integer, 1);
  Types::Fixed real_values("Fixed[R64,1]"_view, real, 1);
  auto& boolean_bytes =
      Constants::Bytes::create_synthetic(domain, booleans, {});
  auto& signed_bytes =
      Constants::Bytes::create_synthetic(domain, signed_values, {});
  auto& real_bytes =
      Constants::Bytes::create_synthetic(domain, real_values, {});
  auto& zero = Constants::Unsigned::create_synthetic(domain, index_type, 0);
  auto& boolean_default = create_slice(domain, boolean_bytes, zero);
  auto& signed_default = create_slice(domain, signed_bytes, zero);
  auto& real_default = create_slice(domain, real_bytes, zero);

  EXPECT(link_expression(domain, boolean_default, source));
  EXPECT(link_expression(domain, signed_default, source));
  EXPECT(link_expression(domain, real_default, source));

  auto boolean_value = selected(boolean_default.fold());
  auto signed_value = selected(signed_default.fold());
  auto real_value = selected(real_default.fold());
  auto signed_payload =
      signed_value ? get_signed(*signed_value) : Option<S64>();
  auto real_payload = real_value ? get_real(*real_value) : Option<R64>();

  ASSERT(boolean_value);
  ASSERT(signed_payload);
  ASSERT(real_payload);
  EXPECT(boolean_value->is_identity<Constants::False>());
  EXPECT(*signed_payload == 0);
  EXPECT(*real_payload == 0.0);
  EXPECT(&boolean_value->get_type() == &boolean);
  EXPECT(&signed_value->get_type() == &signed_integer);
  EXPECT(&real_value->get_type() == &real);
}

PERIMORTEM_UNIT_TEST(LibrarySlice, unsupported_defaults) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U8 unsupported_element;
  const auto& integer = resolve_library_unsigned(source, "U64"_view);
  const auto& signed_integer = resolve_library_signed(source, "S64"_view);
  Types::Fixed fixed("Fixed[U8,1]"_view, unsupported_element, 1);
  auto& bytes = Constants::Bytes::create_synthetic(domain, fixed, "a"_view);
  ValueConstant opaque(fixed);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& one = Constants::Unsigned::create_synthetic(domain, integer, 1);
  auto& negative =
      Constants::Signed::create_synthetic(domain, signed_integer, -1);
  auto& missing = create_slice(domain, bytes, one);
  auto& unsupported = create_slice(domain, opaque, zero);
  auto& safe_range = create_slice(domain, opaque, negative, one);

  EXPECT(link_expression(domain, missing, source));
  EXPECT(link_expression(domain, unsupported, source));
  EXPECT(link_expression(domain, safe_range, source));

  auto missing_value = selected(missing.fold());
  ASSERT(missing_value);
  EXPECT(missing_value->is_identity<Constants::Unsigned>());
  EXPECT(get_unsigned(*missing_value) == Option<U64>(0));
  EXPECT(&missing_value->get_type() == &unsupported_element);
  EXPECT(is_dynamic(unsupported.fold()));
  EXPECT(is_dynamic(safe_range.fold()));
  EXPECT(supplies_self(safe_range, 1));
  EXPECT(&safe_range.get_type() == &unsupported_element);
  EXPECT(&missing.get_type() == &unsupported_element);
  EXPECT(&unsupported.get_type() == &unsupported_element);
}

PERIMORTEM_UNIT_TEST(LibrarySlice, child_failure) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  const auto& element = resolve_library_unsigned(source, "U8"_view);
  const auto& integer = resolve_library_unsigned(source, "U64"_view);
  Types::Fixed fixed("Fixed[U8,1]"_view, element, 1);
  auto& bytes = Constants::Bytes::create_synthetic(domain, fixed, "a"_view);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& one = Constants::Unsigned::create_synthetic(domain, integer, 1);
  ValueFoldOperation failing(domain, zero, one, integer, True);
  auto& access = create_slice(domain, bytes, failing);

  EXPECT(link_expression(domain, access, source));

  auto direct = failing.fold();
  auto propagated = access.fold();
  auto repeated = access.fold();

  EXPECT(reports(direct, Expression::Error::Type::InvalidConstant, failing));
  EXPECT(
      reports(propagated, Expression::Error::Type::InvalidConstant, failing));
  EXPECT(reports(repeated, Expression::Error::Type::InvalidConstant, failing));
}
