// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/static/union.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness CoreUnion = {
  .name = "Core::Static::Union"_view,
};

PERIMORTEM_UNIT_TEST(CoreUnion, null) {
  Static::Union<U32, View::Bytes, Bool> value;

  EXPECT(value.is_null());
  EXPECT(value.find<U32>() == nullptr);
  EXPECT(value.find<View::Bytes>() == nullptr);
}

PERIMORTEM_UNIT_TEST(CoreUnion, alternatives) {
  Static::Union<U32, View::Bytes, Bool> bits(U32(42));
  Static::Union<U32, View::Bytes, Bool> text("open"_view);
  Static::Union<U32, View::Bytes, Bool> flag(True);

  EXPECT_EQ(*bits.find<U32>(), U32(42));
  EXPECT_TEXT(*text.find<View::Bytes>(), "open"_view);
  EXPECT(*flag.find<Bool>());
}

PERIMORTEM_UNIT_TEST(CoreUnion, visit) {
  auto read = [](const auto& source) {
    return source.visit(
        []() { return Count(1); }, [](U32 bits) { return Count(bits); },
        [](View::Bytes text) { return text.get_size(); });
  };

  Static::Union<U32, View::Bytes> empty;
  Static::Union<U32, View::Bytes> bits(U32(42));
  Static::Union<U32, View::Bytes> text("visit"_view);
  EXPECT_EQ(read(empty), Count(1));
  EXPECT_EQ(read(bits), Count(42));
  EXPECT_EQ(read(text), Count(5));

  bits.visit(
      []() {}, [](U32& selected) -> void { selected++; }, [](View::Bytes&) {});
  EXPECT_EQ(*bits.find<U32>(), U32(43));
}

PERIMORTEM_UNIT_TEST(CoreUnion, copy_move) {
  Static::Union<U32, View::Bytes> first("copy"_view);
  Static::Union<U32, View::Bytes> second(first);
  Static::Union<U32, View::Bytes> third(Data::take(second));

  EXPECT_TEXT(*third.find<View::Bytes>(), "copy"_view);
}

PERIMORTEM_UNIT_TEST(CoreUnion, equality) {
  Static::Union<U64> hundred(U64(100));
  Static::Union<U64> another_hundred(U64(100));
  Static::Union<U64> different(U64(101));
  Static::Union<U64> empty;
  Static::Union<U64, View::Bytes> bits(U64(100));
  Static::Union<U64, View::Bytes> text("100"_view);

  EXPECT(hundred == 100);
  EXPECT(hundred == another_hundred);
  EXPECT(hundred != different);
  EXPECT(different != 100);
  EXPECT(empty == Static::Union<U64>());
  EXPECT(empty != hundred);
  EXPECT(bits != text);
}

class ReferencedValue {
 public:
  constexpr ReferencedValue(U64 value) : values{value} {}

  constexpr auto set(U64 replacement) -> void { values[0] = replacement; }
  constexpr auto get() const -> U64 { return values[0]; }

 private:
  U64 values[8];
};

PERIMORTEM_UNIT_TEST(CoreUnion, reference_case) {
  ReferencedValue first(42);
  ReferencedValue equal_value(42);
  Static::Union<ReferencedValue&, U64, Bool> original(first);
  Static::Union<ReferencedValue&, U64, Bool> same(first);
  Static::Union<ReferencedValue&, U64, Bool> distinct(equal_value);
  Static::Union<ReferencedValue&, U64, Bool> copied(original);
  Static::Union<ReferencedValue&, U64, Bool> moved(Data::take(copied));
  const auto& constant = original;

  constant.find<ReferencedValue&>()->set(84);

  EXPECT(original.find<ReferencedValue&>() == &first);
  EXPECT(moved.find<ReferencedValue&>() == &first);
  EXPECT_EQ(first.get(), U64(84));
  EXPECT(original == same);
  EXPECT(original != distinct);
}

static_assert(__is_constructible(Static::Union<U64>, int));
static_assert(!__is_constructible(Static::Union<U64, S64>, int));
static_assert(__is_constructible(
    Static::Union<ReferencedValue&, U64, Bool>,
    ReferencedValue&));
static_assert(!__is_constructible(
    Static::Union<ReferencedValue&, U64, Bool>,
    const ReferencedValue&));
static_assert(!__is_constructible(
    Static::Union<ReferencedValue&, U64, Bool>,
    ReferencedValue));
static_assert(__is_constructible(
    Static::Union<const ReferencedValue&, U64, Bool>,
    const ReferencedValue&));
static_assert(!__is_constructible(
    Static::Union<const ReferencedValue&, U64, Bool>,
    ReferencedValue));
static_assert(__is_trivially_destructible(Static::Union<View::Bytes, S64>));
