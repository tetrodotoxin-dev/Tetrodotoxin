// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/layout.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/interpreter/layout.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "tetrodotoxin/source/layouts/addressable.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness LibraryModelLayout = {
  .name = "Tetrodotoxin::Library::Language::Model::Layout"_view,
};

static auto interpret_source(Workspace& workspace, Errors& errors)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "LayoutModelTest"_view, "layout-model.ttx"_view,
      "// Authored Layout model test.\n"
      "dialect : Library;\n"
      "public Box : struct { public state value : Bool; }\n"
      "public Empty : struct {}"_view);
  BAIL_IF(!interpreted || !interpreted->is<Language::Monograph>());
  return static_cast<Language::Monograph&>(*interpreted);
}

static auto parse_layout(
    Allocator::Arena& arena,
    const Abstract& context,
    Errors& errors,
    View::Bytes text,
    Bool parameters = False) -> Option<Language::Model::Layout&> {
  Tokenizer tokenizer(arena, text, "authored-layout.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto layout = Interpreter::Layout::parse_model(cursor, context, parameters);
  BAIL_IF(!layout || !cursor.matches(Code::Type::Terminal));
  return *layout;
}

PERIMORTEM_UNIT_TEST(LibraryModelLayout, parameter_entries) {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret_source(workspace, errors);
  ASSERT(monograph);

  const Abstract& box = monograph->get_source().resolve_concept("Box"_view);
  ASSERT(box.is<Language::Types::Composite>());

  Allocator::Arena arena;
  Errors parse_errors;
  auto layout = parse_layout(
      arena, *monograph, parse_errors, "[self, .input : Bool,]"_view, True);
  ASSERT(layout);
  Tokenizer link_tokens(
      arena, "[self, .input : Bool,]"_view, "authored-layout.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations link_associations(link_tokens.get_arena());
  Cursor link_cursor(link_tokens, parse_errors, link_associations);
  EXPECT(layout->declares_self());
  EXPECT_NOT(layout->is_linked());
  ASSERT(layout->link_parameters(
      link_cursor, static_cast<const Tetrodotoxin::Source::Type&>(box)));

  ASSERT_EQ(layout->get_size(), Count(2));
  ASSERT(layout->get_name(0) && layout->get_name(1));
  EXPECT_TEXT(*layout->get_name(0), "self"_view);
  EXPECT_TEXT(*layout->get_name(1), "input"_view);

  const Abstract& self = layout->resolve_named("self"_view);
  const Abstract& input = layout->resolve_named("input"_view);
  ASSERT(self.is<Tetrodotoxin::Source::Layouts::Addressable>());
  ASSERT(input.is<Tetrodotoxin::Source::Layouts::Addressable>());
  EXPECT(
      &static_cast<const Tetrodotoxin::Source::Layouts::Addressable&>(self).get_type() ==
      &box);
  EXPECT(
      &static_cast<const Tetrodotoxin::Source::Layouts::Addressable&>(input).get_type() ==
      &monograph->resolve_concept("Bool"_view));
  EXPECT(parse_errors.is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryModelLayout, empty_flow) {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret_source(workspace, errors);
  ASSERT(monograph);

  Allocator::Arena arena;
  Errors parse_errors;
  auto empty = parse_layout(arena, *monograph, parse_errors, "[]"_view);
  auto repeated = parse_layout(arena, *monograph, parse_errors, "[]"_view);
  ASSERT(empty && repeated);

  EXPECT(empty->is_linked());
  EXPECT(repeated->is_linked());
  EXPECT(empty->is_empty());
  EXPECT(repeated->is_empty());
  EXPECT(empty->fits(*repeated));
  EXPECT(repeated->fits(*empty));
  EXPECT(parse_errors.is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryModelLayout, rejects_empty_types) {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret_source(workspace, errors);
  ASSERT(monograph);

  Allocator::Arena arena;
  Errors parse_errors;
  auto named = parse_layout(
      arena, *monograph, parse_errors,
      "[.nothing : Empty, .value : Bool,]"_view);
  ASSERT(named);
  Tokenizer link_tokens(
      arena, "[.nothing : Empty, .value : Bool,]"_view,
      "authored-layout.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations link_associations(link_tokens.get_arena());
  Cursor link_cursor(link_tokens, parse_errors, link_associations);
  EXPECT_NOT(named->is_linked());
  EXPECT_NOT(named->link_types(link_cursor, monograph->get_source()));
  EXPECT_NOT(parse_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryModelLayout, named_fitting) {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret_source(workspace, errors);
  ASSERT(monograph);

  Allocator::Arena arena;
  Errors parse_errors;
  auto source = parse_layout(
      arena, *monograph, parse_errors, "[.flag : Bool, .count : U64]"_view);
  auto reordered = parse_layout(
      arena, *monograph, parse_errors, "[.count : U64, .flag : Bool]"_view);
  ASSERT(source && reordered);
  Tokenizer source_tokens(
      arena, "[.flag : Bool, .count : U64]"_view, "authored-layout.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations source_associations(source_tokens.get_arena());
  Cursor source_cursor(source_tokens, parse_errors, source_associations);
  Tokenizer reordered_tokens(
      arena, "[.count : U64, .flag : Bool]"_view, "authored-layout.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations reordered_associations(
      reordered_tokens.get_arena());
  Cursor reordered_cursor(
      reordered_tokens, parse_errors, reordered_associations);
  ASSERT(source->link_types(source_cursor, monograph->get_source()));
  ASSERT(reordered->link_types(reordered_cursor, monograph->get_source()));

  EXPECT(source->fits(*reordered));
  auto count = source->get_fitted(*reordered, 0);
  auto flag = source->get_fitted(*reordered, 1);
  const Abstract& u64 = monograph->resolve_concept("U64"_view);
  const Abstract& boolean = monograph->resolve_concept("Bool"_view);
  EXPECT(count.visit(
      [&](const Abstract& selected) -> Bool { return Bool(&selected == &u64); },
      [](Tetrodotoxin::Source::Layout::Errors) { return False; }));
  EXPECT(flag.visit(
      [&](const Abstract& selected) -> Bool {
        return Bool(&selected == &boolean);
      },
      [](Tetrodotoxin::Source::Layout::Errors) { return False; }));
  EXPECT(parse_errors.is_empty());
  EXPECT(errors.is_empty());
}
