// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/access.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness LibraryAccess = {
  .name = "Tetrodotoxin::Library::Language::Types::Access"_view,
};

PERIMORTEM_UNIT_TEST(LibraryAccess, direct_contract) {
  Tetrodotoxin::Library::Language::Types::U8 element;
  Types::Access access("Access[U8]"_view, element);

  EXPECT(access.is<Types::Access>());
  EXPECT(access.is<Tetrodotoxin::Source::Type>());
  EXPECT(access.is<Abstract>());
  EXPECT_NOT(access.is<Generic>());
  EXPECT_TEXT(access.get_name(), "Access[U8]"_view);
  EXPECT(&access.get_element_type() == &element);
  EXPECT_NOT(access.get_documentation().is_empty());
  EXPECT(&access.resolve_concept("member"_view) == &None::get_none());
}

PERIMORTEM_UNIT_TEST(LibraryAccess, formula_construction) {
  Allocator::Arena arena;
  Tetrodotoxin::Library::Dialect dialect;
  auto& root = create_library_monograph(arena, dialect);
  Tetrodotoxin::Library::Language::Types::U8 element;
  const auto& formula =
      static_cast<const Generic&>(root.resolve_concept("Access"_view));
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
        auto access = selected.select<Types::Access>();
        return access && selected.get_name() == "Access[U8]"_view &&
                       &access->get_element_type() == &element
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
