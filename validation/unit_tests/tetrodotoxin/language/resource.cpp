// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/resource.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Validation;

class BorrowedResource : public Language::Resource {
 public:
  constexpr BorrowedResource(View::Bytes value) : value(value) {}

  constexpr auto get_value() const -> View::Bytes override { return value; }

 private:
  View::Bytes value;
};

static Harness LanguageResource = {
  .name = "Tetrodotoxin::Language::Resource"_view,
};

PERIMORTEM_UNIT_TEST(LanguageResource, category_contract) {
  BorrowedResource resource("value"_view);
  const Abstract& abstract = resource;

  EXPECT(abstract.is<Language::Resource>());
  EXPECT(abstract.is<Abstract>());
  EXPECT_NOT(abstract.is<Tetrodotoxin::Source::Type>());
  EXPECT_NOT(abstract.is<Unknown>());
  EXPECT_TEXT(resource.get_name(), "Resource"_view);
}

PERIMORTEM_UNIT_TEST(LanguageResource, graph_identity) {
  const View::Bytes value = "same value"_view;
  BorrowedResource first(value);
  BorrowedResource second(value);
  const Abstract& first_abstract = first;

  EXPECT_TEXT(first.get_name(), second.get_name());
  EXPECT_TEXT(first.get_value(), second.get_value());
  EXPECT(&first != &second);
  EXPECT(&first_abstract.resolve() == &first);
  EXPECT(&first_abstract.resolve().resolve() == &first);
}

PERIMORTEM_UNIT_TEST(LanguageResource, context_rejection) {
  U8 binary_route[] = {'x', 0, 'y'};
  const View::Bytes routes[] = {
    {},
    "member"_view,
    "$[resources/value.bin]"_view,
    View::Bytes(binary_route, sizeof(binary_route)),
  };
  BorrowedResource resource("value"_view);
  const None& none = None::get_none();

  for (View::Bytes route : routes) {
    EXPECT(&resource.resolve_concept(route) == &none);
  }
}

PERIMORTEM_UNIT_TEST(LanguageResource, shared_documentation) {
  BorrowedResource resource("value"_view);
  const Tetrodotoxin::Source::Documentation& documentation = resource.get_documentation();

  EXPECT(&documentation == &Tetrodotoxin::Source::Documentation::get_empty());
  EXPECT(documentation.is_empty());
  EXPECT_EQ(documentation.line_count(), 0);
}
