// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/fixed.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/generics/fixed.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness LibraryFixed = {
  .name = "Tetrodotoxin::Library::Language::Types::Fixed"_view,
};

PERIMORTEM_UNIT_TEST(LibraryFixed, direct_contract) {
  Tetrodotoxin::Library::Language::Types::U8 element;
  Types::Fixed fixed("Fixed[U8,4]"_view, element, ::U64(4));
  const Tetrodotoxin::Source::Layouts::Ranged& layout = fixed.get_layout();

  EXPECT(fixed.is<Types::Fixed>());
  EXPECT(fixed.is<Tetrodotoxin::Source::Type>());
  EXPECT(fixed.is<Abstract>());
  EXPECT_NOT(fixed.is<Generic>());
  EXPECT_TEXT(fixed.get_name(), "Fixed[U8,4]"_view);
  EXPECT(&fixed.get_element_type() == &element);
  EXPECT_EQ(fixed.get_extent(), ::U64(4));
  EXPECT_EQ(layout.get_size(), Count(4));
  EXPECT(layout.get_abstract(0).visit(
      []() { return False; },
      [&element](const Abstract& selected) {
        return &selected == &element ? True : False;
      }));
  EXPECT(layout.get_abstract(3).visit(
      []() { return False; },
      [&element](const Abstract& selected) {
        return &selected == &element ? True : False;
      }));
  EXPECT_NOT(layout.get_abstract(4));
  EXPECT_NOT(fixed.get_documentation().is_empty());
  EXPECT(&fixed.resolve_concept("member"_view) == &None::get_none());
}

PERIMORTEM_UNIT_TEST(LibraryFixed, formula_construction) {
  Allocator::Arena arena;
  Tetrodotoxin::Library::Dialect dialect;
  auto& root = create_library_monograph(arena, dialect);
  Tetrodotoxin::Library::Language::Types::U8 element;
  const auto& formula =
      static_cast<const Generic&>(root.resolve_concept("Fixed"_view));
  const Abstract& u8 = root.resolve_concept("U8"_view);
  const Static::Vector<Generic::Argument, 2> positive = {
    {Generic::Argument(element), Generic::Argument(::U64(4))},
  };
  const Static::Vector<Generic::Argument, 2> zero = {
    {Generic::Argument(element), Generic::Argument(::U64(0))},
  };
  const Static::Vector<Generic::Argument, 2> wrong_element = {
    {Generic::Argument(::U64(8)), Generic::Argument(::U64(4))},
  };
  const Static::Vector<Generic::Argument, 2> wrong_extent = {
    {Generic::Argument(element), Generic::Argument(::S64(4))},
  };
  const Static::Vector<Generic::Argument, 1> wrong_arity = {
    {Generic::Argument(element)},
  };

  auto positive_type = formula.materialize(positive);
  EXPECT_NOT(u8.is<Unknown>());
  EXPECT(&formula.resolve_concept("U8"_view) == &Unknown::get_unknown());
  EXPECT(positive_type.visit(
      [&element](const Model::Type& selected) {
        auto fixed = selected.select<Types::Fixed>();
        return fixed && selected.get_name() == "Fixed[U8,4]"_view &&
                       &fixed->get_element_type() == &element &&
                       fixed->get_extent() == ::U64(4)
                   ? True
                   : False;
      },
      [](const Generic::Failure&) { return False; }));

  auto zero_type = formula.materialize(zero);
  EXPECT(zero_type.visit(
      [](const Model::Type&) { return False; },
      [](const Generic::Failure& failure) {
        return failure.get_type() == Generic::Failure::Type::Formula ? True
                                                                     : False;
      }));
  EXPECT(formula.materialize(wrong_element)
             .visit(
                 [](const Model::Type&) { return False; },
                 [](const Generic::Failure& failure) {
                   return failure.get_type() ==
                                      Generic::Failure::Type::Parameter &&
                                  failure.get_argument() == 0
                              ? True
                              : False;
                 }));
  EXPECT(formula.materialize(wrong_extent)
             .visit(
                 [](const Model::Type&) { return False; },
                 [](const Generic::Failure& failure) {
                   return failure.get_type() ==
                                      Generic::Failure::Type::Parameter &&
                                  failure.get_argument() == 1
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
