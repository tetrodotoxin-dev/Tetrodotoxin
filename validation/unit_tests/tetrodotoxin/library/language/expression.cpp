// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/r64.hpp"
#include "tetrodotoxin/library/language/types/s64.hpp"
#include "tetrodotoxin/library/language/types/s8.hpp"
#include "tetrodotoxin/library/language/types/u16.hpp"
#include "tetrodotoxin/library/language/types/u64.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/layouts/named.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Validation;

class ExpressionType : public Model::Type {
 public:
  ExpressionType(View::Bytes name, Tetrodotoxin::Source::Layouts::Named layout = {})
      : name(name), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes) const -> const Abstract& override {
    return Unknown::get_unknown();
  }
  auto get_layout() const -> const Tetrodotoxin::Source::Layouts::Named& override {
    return layout;
  }
  auto create_default(Allocator::Arena&) const
      -> Option<Model::Pack&> override {
    return {};
  }

 private:
  View::Bytes name;
  Tetrodotoxin::Source::Layouts::Named layout;
};

class ExpressionField : public Model::Addressable {
 public:
  ExpressionField(View::Bytes name, const Model::Type& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto get_type() const -> const Model::Type& override { return type; }

 private:
  View::Bytes name;
  const Model::Type& type;
};

class ExpressionValue : public Expression {
 public:
  ExpressionValue(View::Bytes name, const Model::Type& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto get_type() const -> const Model::Type& override { return type; }

 private:
  View::Bytes name;
  const Model::Type& type;
};

// Type selection remains queryable through its exact result but owns no value
// output that fitting or storage may consume.
class ExpressionTypeResult : public Expression {
 public:
  ExpressionTypeResult(const Model::Type& result)
      : Expression({}), result(result) {}

  auto get_name() const -> View::Bytes override { return result.get_name(); }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return result.get_documentation();
  }
  auto get_type() const -> const Abstract& override {
    return Unknown::get_unknown();
  }
  auto get_result() const -> const Abstract& override { return result; }

 private:
  const Model::Type& result;
};

static Harness LibraryExpression = {
  .name = "Tetrodotoxin::Library::Language::Expression"_view,
};

PERIMORTEM_UNIT_TEST(LibraryExpression, address_identity) {
  Allocator::Arena arena;
  Types::Boolean scalar;
  ExpressionField field("value"_view, scalar);
  ExpressionValue receiver("receiver"_view, scalar);
  auto& address =
      Tetrodotoxin::Library::Language::Access::Address::create_synthetic(
          arena, receiver, field);

  EXPECT(address.is<Expression>());
  EXPECT(address.is<Tetrodotoxin::Library::Language::Access::Address>());
  EXPECT_TEXT(address.get_name(), "value"_view);
  EXPECT(&address.get_type() == &scalar);
  EXPECT(&address.get_receiver() == &receiver);
  EXPECT(&address.get_result() == &field);
  EXPECT(address.fits(scalar));
}

PERIMORTEM_UNIT_TEST(LibraryExpression, no_value_flow) {
  Types::Boolean selected;
  ExpressionTypeResult selection(selected);
  Tetrodotoxin::Source::Layouts::Named empty;

  EXPECT(&selection.get_result() == &selected);
  EXPECT(selection.get_layout().is_empty());
  EXPECT(selection.resolve().is<Unknown>());
  EXPECT_NOT(selection.fits(empty));
  EXPECT_NOT(selection.fits(selected));
}

PERIMORTEM_UNIT_TEST(LibraryExpression, constant_identity) {
  Allocator::Arena arena;
  Types::U64 type;
  Types::U64 other_type;
  Types::S64 signed_type;
  auto& first = Constants::Unsigned::create_synthetic(arena, type, ::U64(100));
  auto& same = Constants::Unsigned::create_synthetic(arena, type, ::U64(100));
  auto& different =
      Constants::Unsigned::create_synthetic(arena, type, ::U64(101));
  auto& other =
      Constants::Unsigned::create_synthetic(arena, other_type, ::U64(100));
  auto& signed_value =
      Constants::Signed::create_synthetic(arena, signed_type, ::S64(100));

  EXPECT_NOT(first.is<Expression>());
  EXPECT(first.is<Tetrodotoxin::Source::Constant>());
  EXPECT(first.is<Tetrodotoxin::Library::Language::Constant>());
  EXPECT(first.is<Constants::Unsigned>());
  EXPECT_NOT(first.is<Tetrodotoxin::Source::Type>());
  EXPECT(&first.get_type() == &type);
  EXPECT_TEXT(first.get_name(), "100"_view);
  EXPECT(first.get_value() == 100);
  EXPECT(first == same);
  EXPECT(first != different);
  EXPECT(first != other);
  EXPECT(first != signed_value);

  EXPECT(first.is_complete());
  EXPECT(first.get_identity() && &*first.get_identity() == &first);
}

PERIMORTEM_UNIT_TEST(LibraryExpression, constant_fitting) {
  Allocator::Arena arena;
  Types::U64 u64;
  Types::U8 u8;
  Types::U16 u16;
  Types::S64 s64;
  Types::S8 s8;
  Types::Boolean source_flag;
  Types::Boolean target_flag;
  Types::R64 r64;
  Types::R64 other_r64;
  auto& fits_8 = Constants::Unsigned::create_synthetic(arena, u64, ::U64(255));
  auto& needs_16 =
      Constants::Unsigned::create_synthetic(arena, u64, ::U64(256));
  auto& fits_signed =
      Constants::Signed::create_synthetic(arena, s64, ::S64(-128));
  auto& misses_signed =
      Constants::Signed::create_synthetic(arena, s64, ::S64(-129));
  auto& true_flag = Constants::True::create_synthetic(arena, source_flag);
  auto& false_flag = Constants::False::create_synthetic(arena, source_flag);
  auto& real = Constants::Real::create_synthetic(arena, r64, R64(0.5));

  EXPECT(fits_8.fits(u8));
  EXPECT_NOT(needs_16.fits(u8));
  EXPECT(needs_16.fits(u16));
  EXPECT(fits_signed.fits(s8));
  EXPECT_NOT(misses_signed.fits(s8));
  EXPECT(true_flag.is<Constants::Flag>());
  EXPECT(true_flag.is<Constants::True>());
  EXPECT_NOT(true_flag.is<Constants::False>());
  EXPECT(false_flag.is<Constants::Flag>());
  EXPECT(false_flag.is<Constants::False>());
  EXPECT_NOT(false_flag.is<Constants::True>());
  EXPECT(true_flag.get_value());
  EXPECT_NOT(false_flag.get_value());
  EXPECT(true_flag.fits(target_flag));
  EXPECT(false_flag.fits(target_flag));
  EXPECT(true_flag != false_flag);
  EXPECT(real.fits(r64));
  EXPECT_NOT(real.fits(other_r64));
}

PERIMORTEM_UNIT_TEST(LibraryExpression, byte_lifetime) {
  Allocator::Arena arena;
  ExpressionType type("Bytes"_view);
  ExpressionType other_type("Other Bytes"_view);
  Constants::Bytes* retained = nullptr;

  {
    Dynamic::Bytes source("stable bytes"_view);
    View::Bytes stable = arena.proxy(source);
    retained = &Constants::Bytes::create_synthetic(arena, type, stable);
  }

  auto& same =
      Constants::Bytes::create_synthetic(arena, type, "stable bytes"_view);
  auto& other = Constants::Bytes::create_synthetic(
      arena, other_type, "stable bytes"_view);
  auto& empty = Constants::Bytes::create_synthetic(arena, type, {});
  auto& also_empty = Constants::Bytes::create_synthetic(arena, type, {});

  EXPECT_TEXT(
      retained->get_name(), "$[73 74 61 62 6C 65 20 62 79 74 65 73]"_view);
  EXPECT_TEXT(retained->get_value(), "stable bytes"_view);
  EXPECT(*retained == same);
  EXPECT(*retained != other);
  EXPECT(empty.get_value().is_empty());
  EXPECT(empty == also_empty);
}
