// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/workspace.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/archive/graph_import.hpp"
#include "tetrodotoxin/package/archive/member.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

class WorkspaceMonograph final : public Language::Monograph {
 public:
  TTX_CONTRACT(WorkspaceMonograph, Language::Monograph);

  WorkspaceMonograph(
      Allocator::Arena& arena,
      const Language::Dialect& dialect,
      const Tetrodotoxin::Source::Documentation& documentation,
      Abstract& context,
      View::Bytes fact,
      Span fact_span,
      View::Bytes diagnostic_path)
      : Monograph(arena, dialect, documentation, context),
        fact(fact),
        fact_span(fact_span),
        diagnostic_path(diagnostic_path) {}

  auto link(Cursor& cursor) -> Bool override {
    linked = True;
    if (fact == "link_fail"_view) {
      cursor.create_expression_error(
          fact_span, "Workspace test link failure."_view);
      return False;
    }
    return True;
  }

  auto finalize(Cursor& cursor) -> Bool override {
    if (fact == "finalize_fail"_view) {
      cursor.create_expression_error(
          fact_span, "Workspace test finalization failure."_view);
      return False;
    }
    return True;
  }

  auto link_restored() -> Bool override { return fact != "link_fail"_view; }

  auto finalize_restored() -> Bool override {
    return fact != "finalize_fail"_view;
  }

  auto get_name() const -> View::Bytes override { return "WorkspaceTest"_view; }

  constexpr auto get_fact() const -> View::Bytes { return fact; }

  constexpr auto get_diagnostic_path() const -> View::Bytes {
    return diagnostic_path;
  }

  constexpr auto was_linked() const -> Bool { return linked; }

 private:
  View::Bytes fact;
  Span fact_span;
  View::Bytes diagnostic_path;
  Bool linked = False;
};

class WorkspaceDialect : public Language::Dialect {
 public:
  TTX_NAME("Trace"_view);

  auto interpret(
      Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation,
      const Anchor&,
      Abstract& context) -> Option<Language::Monograph&> override {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_error("Workspace test source requires one fact."_view);
      return {};
    }

    Token fact_token = cursor.current();
    View::Bytes fact = cursor.get_text();
    cursor.consume();
    if (!cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Workspace test source accepts exactly one fact."_view);
      return {};
    }

    if (fact == "reject"_view) {
      cursor.create_expression_error(
          Span(fact_token), "Workspace test interpretation failure."_view);
      return {};
    }

    if (fact == "partial"_view) {
      cursor.create_expression_error(
          Span(fact_token), "Workspace test retained interpretation."_view);
    }

    Allocator::Arena& arena = cursor.get_arena();
    auto& monograph = arena.construct<WorkspaceMonograph>(
        arena, *this, documentation, context, fact, Span(fact_token),
        cursor.get_source_path());
    cursor.get_associations().create(
        Anchor::create(Span(fact_token)), monograph);
    return monograph;
  }

  auto restore(
      Allocator::Arena& arena,
      View::Bytes payload,
      const Tetrodotoxin::Source::Documentation& documentation,
      Abstract& context) -> Option<Language::Monograph&> override {
    View::Bytes fact = arena.proxy(payload);
    auto& monograph = arena.construct<WorkspaceMonograph>(
        arena, *this, documentation, context, fact, Span(), View::Bytes());
    return monograph;
  }
};

class RestoredDialect : public WorkspaceDialect {
 public:
  TTX_NAME("Restored"_view);
};

static Harness EnvironmentWorkspace = {
  .name = "Tetrodotoxin::Environment::Workspace"_view,
  .setup = []() { Diagnostics::Log::set_sink(Test::capture_sink); },
  .teardown =
      []() { Diagnostics::Log::set_sink(Diagnostics::Log::default_sink); },
};

static auto errors_contain(const Errors& errors, View::Bytes text) -> Bool {
  Allocator::Arena arena;
  for (Count index = 0; index < errors.get_size(); index++) {
    if (Algorithm::search(errors.render_message(arena, index), text) !=
        Count(-1)) {
      return True;
    }
  }
  return False;
}

static constexpr View::Bytes source_prefix =
    "// Workspace source.\ndialect : Trace;\n"_view;

static auto make_source(View::Bytes fact) -> Dynamic::Bytes {
  Dynamic::Bytes source(source_prefix);
  source.concat(fact);
  return source;
}

static auto contains_diagnostic(const Errors& errors, View::Bytes fragment)
    -> Bool {
  Allocator::Arena arena;
  for (Count index = 0; index < errors.get_size(); index++) {
    if (Algorithm::search(errors.render_message(arena, index), fragment) !=
        Count(-1)) {
      return True;
    }
  }
  return False;
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, abstract_contract) {
  Environment::Toolchain toolchain;
  Environment::Workspace workspace(toolchain);

  EXPECT_TEXT(workspace.get_name(), "Workspace"_view);
  EXPECT(&workspace.resolve() == &workspace);
  EXPECT(workspace.get_documentation().is_empty());
  EXPECT(&workspace.resolve_concept("missing"_view) == &Unknown::get_unknown());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, unknown_dialect) {
  Environment::Toolchain toolchain;
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "Unknown"_view, "unknown.ttx"_view,
      "// Unknown.\ndialect : Missing;\nvalue"_view);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
  EXPECT(contains_diagnostic(errors, "Unknown dialect Missing"_view));
  EXPECT(&workspace.resolve_concept("Unknown"_view) == &Unknown::get_unknown());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, raw_source_comments) {
  WorkspaceDialect installed_trace;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(installed_trace));
  Environment::Workspace workspace(toolchain);

  Errors accepted_errors;
  auto accepted = workspace.interpret_source(
      accepted_errors, "RawAccepted"_view, "raw-accepted.ttx"_view,
      "/// Build metadata\n"
      "// Visible source documentation.\n"
      "dialect : Trace;\n"
      "complete"_view);
  ASSERT(accepted);
  EXPECT_EQ(accepted->get_documentation().line_count(), Count(1));
  EXPECT_TEXT(
      accepted->get_documentation().get_line(0),
      "Visible source documentation."_view);
  EXPECT(accepted_errors.is_empty());

  Errors rejected_errors;
  auto rejected = workspace.interpret_source(
      rejected_errors, "RawRejected"_view, "raw-rejected.ttx"_view,
      "/// Build metadata\n"
      "dialect : Trace;\n"
      "complete"_view);
  EXPECT_NOT(rejected);
  EXPECT(contains_diagnostic(
      rejected_errors, "Raw comments do not become Documentation"_view));
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, publishes_sources) {
  WorkspaceDialect installed_trace;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(installed_trace));
  Environment::Workspace workspace(toolchain);

  Errors accepted_errors;
  auto accepted_source = make_source("complete"_view);
  auto accepted = workspace.interpret_source(
      accepted_errors, "Accepted"_view, "accepted.ttx"_view, accepted_source);
  ASSERT(accepted && accepted->is<WorkspaceMonograph>());
  EXPECT(&workspace.resolve_concept("Accepted"_view) == &*accepted);
  EXPECT(accepted_errors.is_empty());

  static constexpr View::Bytes failures[] = {
    "reject"_view,
    "link_fail"_view,
    "finalize_fail"_view,
  };
  for (Count index = 0; index < 3; index++) {
    Errors errors;
    Dynamic::Bytes semantic_name("Rejected"_view);
    semantic_name.append('0' + index);
    auto source = make_source(failures[index]);
    auto rejected = workspace.interpret_source(
        errors, semantic_name, "rejected.ttx"_view, source);
    EXPECT_NOT(rejected);
    EXPECT_NOT(errors.is_empty());
    Bool retained =
        workspace.resolve_concept(semantic_name).is<WorkspaceMonograph>();
    EXPECT_EQ(retained, Bool(index != 0));
  }
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, retains_source) {
  WorkspaceDialect installed_trace;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(installed_trace));
  Environment::Workspace workspace(toolchain);
  Dynamic::Bytes path("retained.ttx"_view);
  Dynamic::Bytes source = make_source("retained"_view);
  Errors errors;
  auto interpreted =
      workspace.interpret_source(errors, "Retained"_view, path, source);
  ASSERT(interpreted && interpreted->is<WorkspaceMonograph>());
  const auto& monograph = static_cast<const WorkspaceMonograph&>(*interpreted);

  path.set('x');
  source.set('x');
  EXPECT_TEXT(monograph.get_fact(), "retained"_view);
  EXPECT_TEXT(monograph.get_diagnostic_path(), "retained.ttx"_view);
  EXPECT_TEXT(
      monograph.get_documentation().get_line(0), "Workspace source."_view);

  auto associations = workspace.get_associations(*interpreted);
  ASSERT(associations);
  auto selected = associations->find_at(source_prefix.get_size());
  ASSERT(selected);
  EXPECT(&*selected == &*interpreted);

  Environment::Workspace unrelated(toolchain);
  EXPECT_NOT(unrelated.get_associations(*interpreted));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, links_retained_source) {
  WorkspaceDialect installed_trace;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(installed_trace));
  Environment::Workspace workspace(toolchain);
  auto source = make_source("partial"_view);
  Errors errors;

  auto completed = workspace.interpret_source(
      errors, "Partial"_view, "partial.ttx"_view, source);
  EXPECT_NOT(completed);
  EXPECT_NOT(errors.is_empty());

  auto retained = workspace.get_monograph("partial.ttx"_view);
  ASSERT(retained && retained->is<WorkspaceMonograph>());
  const auto& partial = static_cast<const WorkspaceMonograph&>(*retained);
  EXPECT(partial.was_linked());
  EXPECT_NOT(workspace.get_completed_monograph("partial.ttx"_view));
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, keeps_first_source) {
  WorkspaceDialect installed_trace;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(installed_trace));
  Environment::Workspace workspace(toolchain);
  Errors first_errors;
  auto source = make_source("first"_view);
  auto first = workspace.interpret_source(
      first_errors, "Same"_view, "first.ttx"_view, source);
  ASSERT(first);

  Errors second_errors;
  auto second_source = make_source("second"_view);
  auto second = workspace.interpret_source(
      second_errors, "Same"_view, "second.ttx"_view, second_source);
  EXPECT_NOT(second);
  EXPECT_NOT(second_errors.is_empty());
  EXPECT(&workspace.resolve_concept("Same"_view) == &*first);
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, imports_package) {
  Library::Dialect library;
  Package::Dialect installed_package(library);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(installed_package));
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto imported = workspace.import_package(
      errors, "validation/data/ttx/package_resources"_view, "Resources"_view,
      "package.ttx"_view);
  ASSERT(imported && imported->is<Package::Language::Monograph>());
  const auto& package =
      static_cast<const Package::Language::Monograph&>(*imported);

  ASSERT_EQ(workspace.get_package_source_count(package), Count(3));
  const Abstract& hidden = package.resolve_concept("SharedA"_view).resolve();
  const Abstract& repeated =
      package.resolve_concept("SharedAgain"_view).resolve();
  const Abstract& second = package.resolve_concept("SharedB"_view).resolve();
  const Abstract& deep = package.resolve_concept("Deep"_view).resolve();
  EXPECT(hidden.is<None>());
  EXPECT(repeated.is<Library::Language::Types::Source>());
  EXPECT(second.is<Library::Language::Types::Source>());
  EXPECT(deep.is<Tetrodotoxin::Source::Type>());
  EXPECT_EQ(package.get_resources().get_values().get_size(), Count(2));
  EXPECT(&workspace.resolve_concept("Resources"_view) == &package);
  EXPECT(&workspace.resolve_concept("SharedA"_view) == &Unknown::get_unknown());
  EXPECT(workspace.get_associations(package));
  auto first_source = workspace.get_package_source(package, 0);
  auto second_source = workspace.get_package_source(package, 1);
  ASSERT(first_source && second_source);
  EXPECT(workspace.get_associations(first_source->get_monograph()));
  EXPECT(workspace.get_associations(second_source->get_monograph()));

  auto math = workspace.import_package(
      errors, "packages/ttx/Perimortem.Math"_view, "Math"_view,
      "package.ttx"_view);
  ASSERT(math);
  auto resources_associations = workspace.get_associations(
      "validation/data/ttx/package_resources"_view, "package.ttx"_view);
  auto math_associations = workspace.get_associations(
      "packages/ttx/Perimortem.Math"_view, "package.ttx"_view);
  ASSERT(resources_associations && math_associations);
  EXPECT(&*resources_associations != &*math_associations);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, restores_package) {
  Package::Archive::Member member(
      "Main"_view, "Restored"_view, "restored"_view);
  Package::Archive::GraphImport root_import(
      "PackageSurface"_view, "Main"_view, Language::Import::Kind::Source,
      "Main"_view);
  Package::Archive::Archive archive(
      "Validation.Restored"_view, Version(1, 0), View::Vector(&member, 1), {},
      View::Vector(&root_import, 1));
  Library::Dialect library;
  Package::Dialect installed_package(library);
  RestoredDialect installed_restored;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(installed_package));
  ASSERT(toolchain.install(installed_restored));
  Environment::Workspace workspace(toolchain);

  auto restored = workspace.restore_package(archive, "RestoredPackage"_view);

  ASSERT(restored && restored->is<Package::Language::Monograph>());
  const auto& package =
      static_cast<const Package::Language::Monograph&>(*restored);
  const Abstract& selected = package.resolve_concept("Main"_view).resolve();
  ASSERT(selected.is<WorkspaceMonograph>());
  EXPECT_TEXT(
      static_cast<const WorkspaceMonograph&>(selected).get_fact(),
      "restored"_view);
  EXPECT(&workspace.resolve_concept("RestoredPackage"_view) == &package);

  Package::Archive::Member rejected_member(
      "Main"_view, "Restored"_view, "link_fail"_view);
  Package::Archive::Archive rejected_archive(
      "Validation.Rejected"_view, Version(1, 0),
      View::Vector(&rejected_member, 1));
  Environment::Workspace rejected_workspace(toolchain);

  auto rejected = rejected_workspace.restore_package(
      rejected_archive, "RejectedPackage"_view);

  EXPECT_NOT(rejected);
  EXPECT(
      &rejected_workspace.resolve_concept("RejectedPackage"_view) ==
      &Unknown::get_unknown());

  Package::Archive::Member unfinished_member(
      "Main"_view, "Restored"_view, "finalize_fail"_view);
  Package::Archive::Archive unfinished_archive(
      "Validation.Unfinished"_view, Version(1, 0),
      View::Vector(&unfinished_member, 1));
  Environment::Workspace unfinished_workspace(toolchain);

  auto unfinished = unfinished_workspace.restore_package(
      unfinished_archive, "UnfinishedPackage"_view);

  EXPECT_NOT(unfinished);
  EXPECT(
      &unfinished_workspace.resolve_concept("UnfinishedPackage"_view) ==
      &Unknown::get_unknown());
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, logs_import_errors) {
  Environment::Toolchain toolchain;
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto unopened = workspace.import_package(
      errors, "validation/data/ttx/package_resources/missing-root"_view,
      "Missing"_view, "package.ttx"_view);

  EXPECT_NOT(unopened);
  EXPECT(errors.is_empty());
  EXPECT(
      Test::error_contains(
          "Package import could not open its confined filesystem root"_view));

  auto unreadable = workspace.import_package(
      errors, "validation/data/ttx/package_resources"_view, "Missing"_view,
      "missing-package.ttx"_view);
  EXPECT_NOT(unreadable);
  EXPECT(errors.is_empty());
  EXPECT(
      Test::error_contains(
          "Package root source `missing-package.ttx` could not be read"_view));
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, rejects_path_escape) {
  static constexpr View::Bytes root =
      "validation/data/ttx/package_resources"_view;
  static constexpr View::Bytes source =
      "// Escape test.\n"
      "dialect : Package;\n"
      "package(.name = \"Validation.Escape\", .version = \"1.0\");\n"
      "public Outside : alias = source(\"../outside.ttx\");"_view;
  Library::Dialect library;
  Package::Dialect installed_package(library);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(installed_package));
  Dynamic::Record<Package::Snapshots> snapshots;
  ASSERT(snapshots->overlay(root, "escape.ttx"_view, source));
  Environment::Workspace workspace(toolchain, snapshots);
  Errors errors;
  auto imported =
      workspace.import_package(errors, root, "Escape"_view, "escape.ttx"_view);

  EXPECT_NOT(imported);
  EXPECT(errors_contain(errors, "confined relative path"_view));
}

PERIMORTEM_UNIT_TEST(EnvironmentWorkspace, rejects_direct_package) {
  Library::Dialect library;
  Package::Dialect installed_package(library);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(installed_package));
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "Manifest"_view, "package.ttx"_view,
      "// Package.\ndialect : Package;\n"
      "package(.name = \"Validation.Direct\", .version = \"1.0\");\n"
      "public Main : alias = source(\"main.ttx\");"_view);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
  EXPECT(
      &workspace.resolve_concept("Manifest"_view) == &Unknown::get_unknown());
}
