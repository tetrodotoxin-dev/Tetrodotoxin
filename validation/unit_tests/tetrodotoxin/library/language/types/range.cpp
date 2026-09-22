// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/range.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/generics/range.hpp"
#include "tetrodotoxin/library/language/types/s16.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness LibraryRange = {
  .name = "Tetrodotoxin::Library::Language::Types::Range"_view,
};

PERIMORTEM_UNIT_TEST(LibraryRange, direct_contract) {
  Types::S16 element;
  Types::Range range("Range[S16]"_view, element);

  EXPECT(range.is<Types::Range>());
  EXPECT(range.is<Tetrodotoxin::Source::Type>());
  EXPECT(range.is<Abstract>());
  EXPECT_NOT(range.is<Generic>());
  EXPECT_TEXT(range.get_name(), "Range[S16]"_view);
  EXPECT(&range.get_element_type() == &element);
  ASSERT_EQ(range.get_layout().get_size(), Count(1));
  EXPECT(&*range.get_layout().get_abstract(0) == &range);
  EXPECT_NOT(range.get_documentation().is_empty());
  EXPECT(&range.resolve_concept("member"_view) == &None::get_none());
}

PERIMORTEM_UNIT_TEST(LibraryRange, formula_legality) {
  Allocator::Arena domain;
  Dialect dialect;
  auto& root = create_library_monograph(domain, dialect);
  const auto& formula =
      static_cast<const Generic&>(root.resolve_concept("Range"_view));
  const Static::Vector<Generic::Argument, 1> signed_argument = {{
    Generic::Argument(resolve_library_signed(root, "S8"_view)),
  }};
  const Static::Vector<Generic::Argument, 1> unsigned_argument = {{
    Generic::Argument(resolve_library_unsigned(root, "U64"_view)),
  }};
  const Static::Vector<Generic::Argument, 1> bool_argument = {{
    Generic::Argument(resolve_library_flag(root)),
  }};
  const Static::Vector<Generic::Argument, 1> real_argument = {{
    Generic::Argument(resolve_library_real(root, "R32"_view)),
  }};
  const Static::Vector<Generic::Argument, 1> wrong_kind = {{
    Generic::Argument(U64(1)),
  }};
  View::Vector<Generic::Argument> wrong_arity;

  auto signed_result = formula.materialize(signed_argument);
  auto unsigned_result = formula.materialize(unsigned_argument);
  const Model::Type* signed_type = signed_result.visit(
      [](const Model::Type& selected) { return &selected; },
      [](const Generic::Failure&) -> const Model::Type* { return nullptr; });
  const Model::Type* unsigned_type = unsigned_result.visit(
      [](const Model::Type& selected) { return &selected; },
      [](const Generic::Failure&) -> const Model::Type* { return nullptr; });
  ASSERT(signed_type && unsigned_type);
  EXPECT(signed_type->is<Types::Range>());
  EXPECT(unsigned_type->is<Types::Range>());
  EXPECT(signed_type->visit<Types::Range>(
      [&](const Types::Range& selected) {
        return &selected.get_element_type() ==
                       &resolve_library_signed(root, "S8"_view)
                   ? True
                   : False;
      },
      [](const Abstract&) { return False; }));
  EXPECT(unsigned_type->visit<Types::Range>(
      [&](const Types::Range& selected) {
        return &selected.get_element_type() ==
                       &resolve_library_unsigned(root, "U64"_view)
                   ? True
                   : False;
      },
      [](const Abstract&) { return False; }));
  EXPECT(formula.materialize(bool_argument)
             .visit(
                 [](const Model::Type&) { return False; },
                 [](const Generic::Failure& failure) {
                   return failure.get_type() == Generic::Failure::Type::Formula
                              ? True
                              : False;
                 }));
  EXPECT(formula.materialize(real_argument)
             .visit(
                 [](const Model::Type&) { return False; },
                 [](const Generic::Failure& failure) {
                   return failure.get_type() == Generic::Failure::Type::Formula
                              ? True
                              : False;
                 }));
  EXPECT(formula.materialize(wrong_kind)
             .visit(
                 [](const Model::Type&) { return False; },
                 [](const Generic::Failure& failure) {
                   return failure.get_type() ==
                                      Generic::Failure::Type::Parameter &&
                                  failure.get_argument() == 0
                              ? True
                              : False;
                 }));
  EXPECT(formula.materialize(wrong_arity)
             .visit(
                 [](const Model::Type&) { return False; },
                 [](const Generic::Failure& failure) {
                   return failure.get_type() == Generic::Failure::Type::Arity
                              ? True
                              : False;
                 }));
}

PERIMORTEM_UNIT_TEST(LibraryRange, stable_identity) {
  Allocator::Arena domain;
  Dialect dialect;
  auto& root = create_library_monograph(domain, dialect);
  const auto& formula =
      static_cast<const Generic&>(root.resolve_concept("Range"_view));
  const Static::Vector<Generic::Argument, 1> first_argument = {{
    Generic::Argument(resolve_library_unsigned(root, "U8"_view)),
  }};
  const Static::Vector<Generic::Argument, 1> second_argument = {{
    Generic::Argument(resolve_library_unsigned(root, "U16"_view)),
  }};

  auto first_result = formula.materialize(first_argument);
  auto repeated_result = formula.materialize(first_argument);
  auto second_result = formula.materialize(second_argument);
  const Model::Type* first = first_result.visit(
      [](const Model::Type& selected) { return &selected; },
      [](const Generic::Failure&) -> const Model::Type* { return nullptr; });
  const Model::Type* repeated = repeated_result.visit(
      [](const Model::Type& selected) { return &selected; },
      [](const Generic::Failure&) -> const Model::Type* { return nullptr; });
  const Model::Type* second = second_result.visit(
      [](const Model::Type& selected) { return &selected; },
      [](const Generic::Failure&) -> const Model::Type* { return nullptr; });

  ASSERT(first && repeated && second);
  EXPECT(first == repeated);
  EXPECT(first != second);
  EXPECT_TEXT(first->get_name(), "Range[U8]"_view);
  EXPECT_TEXT(second->get_name(), "Range[U16]"_view);
}
