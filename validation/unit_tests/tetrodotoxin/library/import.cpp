// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/import.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/interpreter/source/import.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Validation;

class ImportContext : public Abstract {
 public:
  ImportContext(Allocator::Arena& arena, View::Bytes name)
      : name(name), bindings(arena) {}

  TTX_CONTRACT(ImportContext, Abstract);
  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  auto bind(View::Bytes local_name, Abstract& target) -> Bool {
    BAIL_IF(local_name.is_empty() || bindings.contains(local_name));
    bindings.launder(local_name, target);
    return True;
  }

  auto resolve_concept(View::Bytes local_name) const
      -> const Abstract& override {
    return bindings.visit(
        local_name,
        [](const Abstract& selected) -> const Abstract& { return selected; },
        []() -> const Abstract& { return Unknown::get_unknown(); });
  }

 private:
  View::Bytes name;
  Managed::Map<View::Bytes, Abstract&> bindings;
};

static auto interpret_library(
    Allocator::Arena& arena,
    Library::Dialect& dialect,
    Abstract& context,
    View::Bytes source,
    Errors& errors) -> Option<Library::Language::Monograph&> {
  Tokenizer tokenizer(arena, source, "library-import.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  Count error_count = errors.get_size();
  auto interpretation = dialect.interpret(
      cursor, Tetrodotoxin::Source::Documentation::get_empty(), Anchor::create(Span()), context);
  BAIL_IF(
      !interpretation || errors.get_size() != error_count ||
      !interpretation->is<Library::Language::Monograph>() ||
      !cursor.matches(Code::Type::Terminal));
  return static_cast<Library::Language::Monograph&>(*interpretation);
}

static auto complete_library(
    Allocator::Arena& arena,
    Library::Language::Monograph& monograph,
    View::Bytes source,
    Errors& errors) -> Bool {
  Tokenizer tokenizer(arena, source, "library-import.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  return monograph.link(cursor) && monograph.finalize(cursor);
}

static auto diagnostic_contains(const Errors& errors, View::Bytes text)
    -> Bool {
  Allocator::Arena rendered;
  for (Count index = 0; index < errors.get_size(); index++) {
    if (Algorithm::search(errors.render_message(rendered, index), text) !=
        Count(-1)) {
      return True;
    }
  }
  return False;
}

static Harness LibraryImports = {
  .name = "Tetrodotoxin::Library::Language::Import"_view,
};

PERIMORTEM_UNIT_TEST(LibraryImports, statement_grammar) {
  static constexpr Static::Vector<View::Bytes, 2> accepted = {{
    "using Core;"_view,
    "using Runtime::Core::Api;"_view,
  }};
  for (Count index = 0; index < accepted.get_size(); index++) {
    View::Bytes source = accepted[index];
    Allocator::Arena arena;
    Errors errors;
    Tokenizer tokenizer(arena, source, "import.ttx"_view);
    Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
    auto import = Library::Interpreter::Source::Import::parse(
        cursor, Tetrodotoxin::Source::Documentation::get_empty());
    EXPECT(import && cursor.matches(Code::Type::Terminal));
    EXPECT(errors.is_empty());
  }

  static constexpr Static::Vector<View::Bytes, 8> rejected = {{
    "Using Core;"_view,
    "using core;"_view,
    "using Runtime ::Core;"_view,
    "using Runtime:: Core;"_view,
    "using;"_view,
    "using Core Other;"_view,
    "using Core"_view,
    "using Core[];"_view,
  }};
  for (Count index = 0; index < rejected.get_size(); index++) {
    View::Bytes source = rejected[index];
    Allocator::Arena arena;
    Errors errors;
    Tokenizer tokenizer(arena, source, "import.ttx"_view);
    Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
    EXPECT_NOT(
        Library::Interpreter::Source::Import::parse(
            cursor, Tetrodotoxin::Source::Documentation::get_empty()));
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(LibraryImports, selected_fallback) {
  static constexpr View::Bytes provider_source =
      "// Provider.\n"
      "public Provided : struct { public state ready : Bool; }\n"
      "private Hidden : struct { private state ready : Bool; }"_view;
  static constexpr View::Bytes importer_source =
      "// Importer.\n"
      "using Runtime::Core::Provider;\n"
      "public Local : struct { public state value : Provided; }"_view;

  Allocator::Arena arena;
  Library::Dialect library;
  ImportContext context(arena, "Root"_view);
  Errors errors;
  auto provider =
      interpret_library(arena, library, context, provider_source, errors);
  ASSERT(provider);
  ASSERT(complete_library(arena, *provider, provider_source, errors));

  ImportContext target(arena, "Core"_view);
  ASSERT(target.bind("Provider"_view, *provider));
  ImportContext runtime(arena, "Runtime"_view);
  ASSERT(runtime.bind("Core"_view, target));
  ImportContext source(arena, "Source"_view);
  ASSERT(source.bind("Runtime"_view, runtime));

  auto importer =
      interpret_library(arena, library, source, importer_source, errors);
  ASSERT(importer);
  ASSERT(complete_library(arena, *importer, importer_source, errors));

  const Abstract& provided = provider->resolve_concept("Provided"_view);
  EXPECT(
      &importer->resolve_concept("Provided"_view).resolve() ==
      &provided.resolve());
  EXPECT(importer->resolve_concept("Hidden"_view).is<Unknown>());
  auto local_types = importer->get_source().get_types();
  ASSERT(local_types != local_types.end());
  EXPECT_TEXT((*local_types).get().get_name(), "Local"_view);
  ++local_types;
  EXPECT(local_types == importer->get_source().get_types().end());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryImports, fallback_composition) {
  static constexpr View::Bytes upstream_source =
      "// Upstream.\n"
      "public Upstream : struct { public state ready : Bool; }"_view;
  static constexpr View::Bytes provider_source =
      "// Provider.\n"
      "using Up::Api;\n"
      "public Provider : struct { public state ready : Bool; }"_view;
  static constexpr View::Bytes importer_source =
      "// Importer.\n"
      "using Provider;\n"
      "public Local : struct { public state value : Upstream; }"_view;

  Allocator::Arena arena;
  Library::Dialect library;
  ImportContext context(arena, "Root"_view);
  Errors errors;
  auto upstream =
      interpret_library(arena, library, context, upstream_source, errors);
  ASSERT(upstream);
  ASSERT(complete_library(arena, *upstream, upstream_source, errors));
  ImportContext upstream_context(arena, "Upstream"_view);
  ASSERT(upstream_context.bind("Api"_view, *upstream));

  ImportContext provider_context(arena, "ProviderContext"_view);
  ASSERT(provider_context.bind("Up"_view, upstream_context));
  auto provider = interpret_library(
      arena, library, provider_context, provider_source, errors);
  ASSERT(provider);
  ASSERT(complete_library(arena, *provider, provider_source, errors));

  ImportContext importer_context(arena, "ImporterContext"_view);
  ASSERT(importer_context.bind("Provider"_view, *provider));
  auto importer = interpret_library(
      arena, library, importer_context, importer_source, errors);
  ASSERT(importer);
  ASSERT(complete_library(arena, *importer, importer_source, errors));

  const Abstract& upstream_type = upstream->resolve_concept("Upstream"_view);
  EXPECT(
      &importer->resolve_concept("Upstream"_view).resolve() ==
      &upstream_type.resolve());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryImports, local_collision) {
  static constexpr View::Bytes provider_source =
      "// Provider.\n"
      "public Shared : struct { public state ready : Bool; }"_view;
  static constexpr View::Bytes importer_source =
      "// Importer.\n"
      "using Provider;\n"
      "public Shared : struct { public state local : Bool; }"_view;

  Allocator::Arena arena;
  Library::Dialect library;
  ImportContext context(arena, "Root"_view);
  Errors errors;
  auto provider =
      interpret_library(arena, library, context, provider_source, errors);
  ASSERT(provider);
  ASSERT(complete_library(arena, *provider, provider_source, errors));
  ImportContext package_context(arena, "ImporterContext"_view);
  ASSERT(package_context.bind("Provider"_view, *provider));
  auto importer = interpret_library(
      arena, library, package_context, importer_source, errors);
  ASSERT(importer);
  EXPECT_NOT(complete_library(arena, *importer, importer_source, errors));
  EXPECT(
      diagnostic_contains(errors, "conflicts with this source context"_view));
}

PERIMORTEM_UNIT_TEST(LibraryImports, route_diagnostics) {
  static constexpr View::Bytes provider_source =
      "// Provider.\n"
      "public Provided : struct { public state ready : Bool; }"_view;
  static constexpr Static::Vector<View::Bytes, 2> rejected = {{
    "// Missing.\n"
    "using Missing;"_view,
    "// Duplicate.\n"
    "using Provider;\n"
    "using Provider;"_view,
  }};

  for (Count index = 0; index < rejected.get_size(); index++) {
    Allocator::Arena arena;
    Library::Dialect library;
    ImportContext context(arena, "Root"_view);
    Errors errors;
    auto provider =
        interpret_library(arena, library, context, provider_source, errors);
    ASSERT(provider);
    ASSERT(complete_library(arena, *provider, provider_source, errors));
    ImportContext package_context(arena, "ImporterContext"_view);
    ASSERT(package_context.bind("Provider"_view, *provider));

    auto importer = interpret_library(
        arena, library, package_context, rejected[index], errors);
    ASSERT(importer);
    EXPECT_NOT(complete_library(arena, *importer, rejected[index], errors));
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(LibraryImports, ambiguous_fallback) {
  static constexpr View::Bytes provider_source =
      "// Provider.\n"
      "public Shared : struct { public state ready : Bool; }"_view;
  static constexpr View::Bytes importer_source =
      "// Importer.\n"
      "using First;\n"
      "using Second;"_view;

  Allocator::Arena arena;
  Library::Dialect library;
  ImportContext context(arena, "Root"_view);
  Errors errors;
  auto first =
      interpret_library(arena, library, context, provider_source, errors);
  auto second =
      interpret_library(arena, library, context, provider_source, errors);
  ASSERT(first && second);
  ASSERT(complete_library(arena, *first, provider_source, errors));
  ASSERT(complete_library(arena, *second, provider_source, errors));

  ImportContext package_context(arena, "ImporterContext"_view);
  ASSERT(package_context.bind("First"_view, *first));
  ASSERT(package_context.bind("Second"_view, *second));
  auto importer = interpret_library(
      arena, library, package_context, importer_source, errors);
  ASSERT(importer);
  ASSERT(complete_library(arena, *importer, importer_source, errors));
  EXPECT(importer->resolve_concept("Shared"_view).is<Unknown>());
  EXPECT(errors.is_empty());
}
