// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/view.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness LibraryView = {
  .name = "Tetrodotoxin::Library::Language::Types::View"_view,
};

PERIMORTEM_UNIT_TEST(LibraryView, direct_contract) {
  Tetrodotoxin::Library::Language::Types::U8 element;
  Types::View view("View[U8]"_view, element);

  EXPECT(view.is<Types::View>());
  EXPECT(view.is<Tetrodotoxin::Source::Type>());
  EXPECT(view.is<Abstract>());
  EXPECT_NOT(view.is<Generic>());
  EXPECT_TEXT(view.get_name(), "View[U8]"_view);
  EXPECT(&view.get_element_type() == &element);
  EXPECT_NOT(view.get_documentation().is_empty());
  EXPECT(&view.resolve_concept("member"_view) == &None::get_none());
}

PERIMORTEM_UNIT_TEST(LibraryView, formula_construction) {
  Allocator::Arena arena;
  Tetrodotoxin::Library::Dialect dialect;
  auto& root = create_library_monograph(arena, dialect);
  Tetrodotoxin::Library::Language::Types::U8 element;
  const auto& formula =
      static_cast<const Generic&>(root.resolve_concept("View"_view));
  const Static::Vector<Generic::Argument, 1> accepted = {
    {Generic::Argument(element)},
  };
  const Static::Vector<Generic::Argument, 1> wrong_category = {
    {Generic::Argument(::U64(8))},
  };
  View::Vector<Generic::Argument> wrong_arity;

  auto created = formula.materialize(accepted);
  EXPECT(created.visit(
      [&element](const Model::Type& selected) {
        auto view = selected.select<Types::View>();
        return view && selected.get_name() == "View[U8]"_view &&
                       &view->get_element_type() == &element
                   ? True
                   : False;
      },
      [](const Generic::Failure&) { return False; }));
  EXPECT(formula.materialize(wrong_category)
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
