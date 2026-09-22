// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/foreign.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Validation;

using Tetrodotoxin::Language::Visibility;

static Harness ForeignTests = {
  .name = "Tetrodotoxin::Library::Language::Foreign"_view,
};

static auto interpret(
    Environment::Workspace& workspace,
    Errors& errors,
    View::Bytes source) -> Option<Library::Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "ForeignTest"_view, "foreign.ttx"_view, source);
  if (!interpreted || !interpreted->is<Library::Language::Monograph>()) {
    return {};
  }
  return static_cast<Library::Language::Monograph&>(*interpreted);
}

static auto rejects(View::Bytes source, View::Bytes expected) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Environment::Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (monograph || errors.is_empty() ||
      !retains_library_source(workspace, "ForeignTest"_view)) {
    return False;
  }

  Perimortem::Memory::Allocator::Arena rendered_domain;
  View::Bytes rendered = errors.render_message(rendered_domain, 0);
  return expected.is_empty() ||
         Algorithm::search(rendered, expected) != Count(-1);
}

PERIMORTEM_UNIT_TEST(ForeignTests, source_lifecycle) {
  static constexpr View::Bytes source =
      "// Foreign source identity.\n"
      "dialect : Library;\n"
      "// Primary Foreign context.\n"
      "foreign \"C\" {\n"
      "  // Shared State category.\n"
      "  public state shared : U64;\n"
      "  expose state observed : U64;\n"
      "  public state buffer : Fixed[U8, 4];\n"
      "  // Shared Callable category.\n"
      "  public func transform[.value : U64] -> U64;\n"
      "  public func notify[] -> [];\n"
      "}\n"
      "// Extended Foreign context.\n"
      "foreign \"C\" {\n"
      "  public state shared : U64;\n"
      "}\n"_view;

  Tetrodotoxin::Library::Dialect workspace_toolchain_library;

  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);

  Environment::Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  const Library::Language::Foreign& foreign =
      monograph->get_source().get_foreign();
  ASSERT(foreign.get_abi());
  EXPECT(*foreign.get_abi() == "C"_view);
  EXPECT_TEXT(
      foreign.get_documentation().get_line(0), "Primary Foreign context."_view);
  EXPECT_TEXT(
      foreign.get_documentation().get_line(1),
      "Extended Foreign context."_view);
  EXPECT(&monograph->resolve_concept("foreign"_view) == &foreign);
  EXPECT(&monograph->get_source().resolve_concept("foreign"_view) == &foreign);
  EXPECT(monograph->resolve_concept("shared"_view).is<Unknown>());

  const Abstract& shared_identity =
      foreign.resolve_concept("static"_view).resolve_concept("shared"_view);
  const Abstract& observed_identity =
      foreign.resolve_concept("static"_view).resolve_concept("observed"_view);
  const Abstract& buffer_identity =
      foreign.resolve_concept("static"_view).resolve_concept("buffer"_view);
  const Abstract& transform_identity =
      foreign.resolve_concept("static"_view).resolve_concept("transform"_view);
  const Abstract& notify_identity =
      foreign.resolve_concept("static"_view).resolve_concept("notify"_view);
  EXPECT_EQ(foreign.get_states().get_size(), Count(3));
  EXPECT_EQ(foreign.get_functions().get_size(), Count(2));

  ASSERT(shared_identity.is<Library::Language::Foreign::State>());
  ASSERT(observed_identity.is<Library::Language::Foreign::State>());
  ASSERT(buffer_identity.is<Library::Language::Foreign::State>());
  ASSERT(transform_identity.is<Library::Language::Foreign::Function>());
  ASSERT(notify_identity.is<Library::Language::Foreign::Function>());
  const auto& shared_state =
      static_cast<const Library::Language::Foreign::State&>(shared_identity);
  const auto& observed =
      static_cast<const Library::Language::Foreign::State&>(observed_identity);
  const auto& buffer =
      static_cast<const Library::Language::Foreign::State&>(buffer_identity);
  const auto& transform =
      static_cast<const Library::Language::Foreign::Function&>(
          transform_identity);
  const auto& notify =
      static_cast<const Library::Language::Foreign::Function&>(notify_identity);

  EXPECT(&shared_state.get_type() == &monograph->resolve_concept("U64"_view));
  const auto& state_definition = shared_state.get_definition();
  EXPECT(state_definition.get_visibility() == Visibility::Public);
  EXPECT(observed.get_definition().get_visibility() == Visibility::Exposed);
  EXPECT(state_definition.get_authored().is_authored());
  EXPECT(&state_definition.get_host() == &foreign);
  EXPECT_TEXT(
      state_definition.get_documentation().get_line(0),
      "Shared State category."_view);
  EXPECT_TEXT(
      state_definition.get_authored().get_anchor().get_span().caculate_text(source),
      "public state shared : U64;"_view);
  EXPECT(buffer.get_type().is<Library::Language::Types::Fixed>());
  EXPECT(buffer.get_type_reference().has_arguments());
  EXPECT(shared_state.get_abi() == "C"_view);

  EXPECT(transform.get_abi() == "C"_view);
  EXPECT(transform.get_symbol() == "transform"_view);
  const auto& function_definition = transform.get_definition();
  EXPECT(function_definition.get_visibility() == Visibility::Public);
  EXPECT(function_definition.get_authored().is_authored());
  EXPECT(&function_definition.get_host() == &foreign);
  EXPECT_TEXT(
      function_definition.get_documentation().get_line(0),
      "Shared Callable category."_view);
  EXPECT_TEXT(
      function_definition.get_authored().get_anchor().get_span().caculate_text(source),
      "public func transform[.value : U64] -> U64;"_view);
  EXPECT_EQ(transform.get_parameters().get_size(), Count(1));
  EXPECT_EQ(transform.get_results().get_size(), Count(1));
  EXPECT_EQ(notify.get_parameters().get_size(), Count(0));
  EXPECT_EQ(notify.get_results().get_size(), Count(0));
  EXPECT_NOT(transform.is_type_bound());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ForeignTests, foreign_categories) {
  static constexpr View::Bytes source =
      "// Foreign access integration.\n"
      "dialect : Library;\n"
      "public shared : U64 = 7;\n"
      "foreign \"C\" {\n"
      "  expose state input : U64;\n"
      "  public state output : U64;\n"
      "  public state shared : U64;\n"
      "  public func shared[.value : U64] -> U64;\n"
      "  public func no_result[] -> [];\n"
      "}\n"
      "public consume : func = [] -> U64 {\n"
      "  foreign.output = foreign.input;\n"
      "  foreign -> no_result();\n"
      "  return foreign -> shared(foreign.output);\n"
      "}\n"_view;

  Tetrodotoxin::Library::Dialect workspace_toolchain_library;

  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);

  Environment::Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  EXPECT_NOT(monograph);
  EXPECT_NOT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ForeignTests, authored_rejections) {
  struct Rejection {
    View::Bytes source;
    View::Bytes message;
  };
  static constexpr Static::Vector<Rejection, 9> rejected = {{
    Rejection{
      "// ABI.\ndialect : Library;\nforeign \"C++\" {}"_view,
      "Foreign supports only the exact `\"C\"` ABI selector."_view},
    {"// Named.\ndialect : Library;\nprivate C : foreign {}"_view,
     "Library members require a Type"_view},
    {"// Const.\ndialect : Library;\nforeign \"C\" { public const value : "
     "U64; }"_view,
     "Foreign const declarations require a loader or embedding contract."_view},
    {"// Private State.\ndialect : Library;\nforeign \"C\" { private state "
     "value : U64; }"_view,
     "Private Foreign State is unreachable from its parent Library."_view},
    {"// Private Function.\ndialect : Library;\nforeign \"C\" { private func "
     "call[] -> []; }"_view,
     "Private Foreign Functions are unreachable from their parent Library."_view},
    {"// Exposed Function.\ndialect : Library;\nforeign \"C\" { expose func "
     "call[] -> []; }"_view,
     "Foreign Functions do not accept `expose` visibility."_view},
    {"// Receiver.\ndialect : Library;\nforeign \"C\" { public func call[self] "
     "-> []; }"_view,
     "Foreign Function cannot declare a `self` receiver."_view},
    {"// Body.\ndialect : Library;\nforeign \"C\" { public func call[] -> "
     "[] {} }"_view,
     "Foreign Function declarations cannot contain an authored body."_view},
    {"// Conflict.\ndialect : Library;\nforeign \"C\" { public state value : "
     "U64; public state value : Bool; }"_view,
     "Repeated Foreign State changes its declaration."_view},
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    EXPECT(rejects(rejected[i].source, rejected[i].message));
  }
}

PERIMORTEM_UNIT_TEST(ForeignTests, link_rejections) {
  struct Rejection {
    View::Bytes source;
    View::Bytes message;
  };
  static constexpr Static::Vector<Rejection, 8> rejected = {{
    Rejection{
      "// Empty State.\ndialect : Library;\npublic Empty : struct {} foreign "
      "\"C\" { public state value : Empty; }"_view,
      "Foreign State cannot bind an empty Type Layout."_view},
    {"// Exposed write.\ndialect : Library;\nforeign \"C\" { expose state "
     "value "
     ": U64; } public write : func = [] -> [] { foreign.value = 1; "
     "return; }"_view,
     "Write source Pack is incompatible with this receiving Expression."_view},
    {"// Missing State.\ndialect : Library;\nforeign \"C\" {} public read : "
     "func = [] -> U64 { return foreign.missing; }"_view,
     "Address did not find one readable Addressable"_view},
    {"// Ambient.\ndialect : Library;\nprivate foreign : U64; foreign "
     "\"C\" {} public read : func = [] -> U64 { return "
     "foreign.missing; }"_view,
     "Address did not find one readable Addressable"_view},
    {"// Missing Function.\ndialect : Library;\nforeign \"C\" {} public call : "
     "func = [] -> [] { foreign -> missing(); return; }"_view,
     "Library invocation did not resolve one accessible Callable."_view},
    {"// Arguments.\ndialect : Library;\nforeign \"C\" { public func "
     "use[.value "
     ": U64] -> []; } public call : func = [] -> [] { foreign -> "
     "use(false); return; }"_view,
     "Library invocation arguments do not fit"_view},
    {"// Dot mismatch.\ndialect : Library;\nforeign \"C\" { public func "
     "shared[] -> []; } public read : func = [] -> [] { return "
     "foreign.shared; }"_view,
     "Address did not find one readable Addressable"_view},
    {"// Arrow mismatch.\ndialect : Library;\nforeign \"C\" { public state "
     "shared : U64; } public call : func = [] -> [] { foreign -> "
     "shared(); return; }"_view,
     "Library invocation did not resolve one accessible Callable."_view},
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    EXPECT(rejects(rejected[i].source, {}));
  }
}
