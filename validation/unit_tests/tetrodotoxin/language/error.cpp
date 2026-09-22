// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/error.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

enum class TestCause : U8 {
  Unknown = U8(-1),
  Unreadable = 0,
};

class ContextError : public Language::Error {
 public:
  constexpr ContextError(TestCause cause, View::Bytes context)
      : cause(cause), context(context) {}

  auto describe(Errors::Report& report) const -> void override {
    switch (cause) {
    case TestCause::Unreadable:
      report << "Unable to acquire "_view << context << "."_view;
      break;
    case TestCause::Unknown:
      report << "Unable to acquire an unknown resource."_view;
      break;
    }

    report.get_hint() << "Check the package contents."_view;
  }

 private:
  TestCause cause;
  View::Bytes context;
};

static Harness LanguageError = {
  .name = "Tetrodotoxin::Language::Error"_view,
};

PERIMORTEM_UNIT_TEST(LanguageError, category_contract) {
  ContextError error(TestCause::Unreadable, "resources/table.bin"_view);
  const Abstract& abstract = error;

  EXPECT(abstract.is<Language::Error>());
  EXPECT(abstract.is<Abstract>());
  EXPECT_NOT(abstract.is<Language::Resource>());
  EXPECT_NOT(abstract.is<Unknown>());
  EXPECT_NOT(abstract.is<Tetrodotoxin::Source::Type>());
  EXPECT_TEXT(error.get_name(), "Error"_view);

  const Tetrodotoxin::Source::Documentation& documentation = error.get_documentation();
  EXPECT(&documentation == &Tetrodotoxin::Source::Documentation::get_empty());
  EXPECT(documentation.is_empty());
  EXPECT_EQ(documentation.line_count(), 0);
}

PERIMORTEM_UNIT_TEST(LanguageError, stable_identity) {
  const View::Bytes context = "resources/table.bin"_view;
  ContextError first(TestCause::Unreadable, context);
  ContextError second(TestCause::Unreadable, context);
  const Abstract& first_abstract = first;
  const Unknown& invalid = Unknown::get_unknown();
  const Abstract& invalid_abstract = invalid;

  EXPECT(&first != &second);
  EXPECT(&first_abstract.resolve() == &first);
  EXPECT(&first_abstract.resolve() == &first_abstract.resolve());
  EXPECT(&first_abstract.resolve().resolve() == &first);
  EXPECT(&first_abstract != &invalid_abstract);
  EXPECT(first_abstract.is<Language::Error>());
  EXPECT_NOT(invalid.is<Language::Error>());
  EXPECT_TEXT(first.get_name(), second.get_name());
}

PERIMORTEM_UNIT_TEST(LanguageError, context_rejection) {
  U8 binary_route[] = {'x', 0, 'y'};
  const View::Bytes routes[] = {
    {},
    "member"_view,
    "$[resources/value.bin]"_view,
    View::Bytes(binary_route, sizeof(binary_route)),
  };
  ContextError error(TestCause::Unreadable, "resources/table.bin"_view);
  const None& none = None::get_none();

  for (View::Bytes route : routes) {
    EXPECT(&error.resolve_concept(route) == &none);
  }
}

PERIMORTEM_UNIT_TEST(LanguageError, consumer_context) {
  static constexpr View::Bytes source_name = "member.ttx"_view;
  static constexpr View::Bytes source_text =
      "prefix\nsecond\nconst value = $[resources/table.bin];"_view;
  static constexpr View::Bytes alternate_name = "alternate.ttx"_view;
  static constexpr View::Bytes alternate_text =
      "load $[resources/table.bin]"_view;
  const Span source_span(Token(28, 3, 15, 21, Code::Type::Embedded));
  const Span alternate_span(Token(5, 1, 6, 21, Code::Type::Embedded));
  Allocator::Arena render_arena;
  Errors errors;
  ContextError error(TestCause::Unreadable, "resources/table.bin"_view);

  // The consumer chooses each authored range. Error sees only the finished
  // Report so its route facts cannot masquerade as source provenance.
  {
    Errors::Report report(
        errors, source_name, source_text, Anchor::create(source_span));
    error.describe(report);
  }

  {
    Errors::Report report(
        errors, alternate_name, alternate_text, Anchor::create(alternate_span));
    error.describe(report);
  }

  View::Bytes rendered = errors.render_message(render_arena, 0);
  View::Bytes alternate = errors.render_message(render_arena, 1);

  ASSERT_EQ(errors.get_size(), Count(2));
  EXPECT(
      Algorithm::search(
          rendered, "Unable to acquire resources/table.bin."_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(rendered, "Check the package contents."_view) !=
      Count(-1));
  EXPECT(Algorithm::search(rendered, "member.ttx:3:15"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          rendered, "const value = $[resources/table.bin];"_view) != Count(-1));
  EXPECT(Algorithm::search(alternate, "alternate.ttx:1:6"_view) != Count(-1));
  EXPECT(
      Algorithm::search(alternate, "load $[resources/table.bin]"_view) !=
      Count(-1));
  EXPECT(Algorithm::search(alternate, "member.ttx"_view) == Count(-1));
}
