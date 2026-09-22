// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/branch.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/flow/loop_control.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness BranchTests = {
  .name = "Tetrodotoxin::Library::Language::Flow::Branch"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "BranchTest"_view, "branch.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  for (const Reference<Abstract>& binding : composite.get_callables()) {
    if (binding.get().get_name() == name &&
        binding.get().is<Language::Function>()) {
      return static_cast<const Language::Function&>(binding.get());
    }
  }

  return {};
}

static auto rejects_link(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return !monograph && !errors.is_empty();
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  return !interpret(workspace, errors, source) && !errors.is_empty() &&
         retains_library_source(workspace, "BranchTest"_view);
}

PERIMORTEM_UNIT_TEST(BranchTests, branch_shape) {
  static constexpr View::Bytes source =
      "// Branch graph.\n"
      "dialect : Library;\n"
      "public run : func = [] -> U64 {\n"
      "  state outer : U64 = 1;\n"
      "  if (true, 9) {\n"
      "    state inner : U64 = 3;\n"
      "    inner += 1;\n"
      "  } else {\n"
      "    outer = 2;\n"
      "  }\n"
      "  while false {\n"
      "    outer += 1;\n"
      "  }\n"
      "  return outer;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "run"_view);
  ASSERT(function && function->get_body());
  auto statements = function->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(4));
  ASSERT(statements.get_data()[0].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[1].get_root().is<Language::Flow::Branch>());
  ASSERT(statements.get_data()[2].get_root().is<Language::Flow::Branch>());
  ASSERT(statements.get_data()[3].get_root().is<Language::Flow::Return>());

  const auto& conditional = static_cast<const Language::Flow::Branch&>(
      statements.get_data()[1].get_root());
  const auto& loop = static_cast<const Language::Flow::Branch&>(
      statements.get_data()[2].get_root());
  EXPECT(conditional.get_kind() == Language::Flow::Branch::Kind::If);
  EXPECT(loop.get_kind() == Language::Flow::Branch::Kind::While);
  EXPECT_EQ(conditional.get_condition().get_layout().get_size(), Count(2));
  ASSERT(conditional.get_alternate());
  EXPECT_TEXT(
      conditional.get_anchor().get_span().caculate_text(source),
      "if (true, 9) {\n"
      "    state inner : U64 = 3;\n"
      "    inner += 1;\n"
      "  } else {\n"
      "    outer = 2;\n"
      "  }"_view);

  const Abstract& outer = statements.get_data()[0].get_root();
  EXPECT(&conditional.get_body().resolve_concept("outer"_view) == &outer);
  const Abstract& inner = conditional.get_body().resolve_concept("inner"_view);
  EXPECT(inner.is<Language::Flow::Local>());
  auto alternate =
      conditional.get_alternate()->get_root().select<Language::Flow::Block>();
  ASSERT(alternate);
  EXPECT(&alternate->resolve_concept("outer"_view) == &outer);

  const Abstract& retained = statements.get_data()[1].get_root();
  Perimortem::Memory::Allocator::Arena transaction;
  Tokenizer tokenizer(transaction, source, "branch.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph->link(cursor));
  ASSERT(monograph->finalize(cursor));
  EXPECT(
      &function->get_body()->get_statements().get_data()[1].get_root() ==
      &retained);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(BranchTests, terminal_if) {
  static constexpr View::Bytes source =
      "// Terminal branch.\n"
      "dialect : Library;\n"
      "public select : func = [.flag : Bool] -> U64 {\n"
      "  if flag {\n"
      "    return 1;\n"
      "  } else {\n"
      "    return 2;\n"
      "  }\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "select"_view);
  ASSERT(function && function->get_body());
  EXPECT_NOT(function->get_body()->reaches_next_statement());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(BranchTests, else_if) {
  static constexpr View::Bytes source =
      "// Else if statement.\n"
      "dialect : Library;\n"
      "public select : func = [.first : Bool, .second : Bool] -> U64 "
      "{\n"
      "  if first : return 1; else if second : return 2; else : return 3;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "select"_view);
  ASSERT(function && function->get_body());
  auto statements = function->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(1));
  auto first =
      statements.get_data()[0].get_root().select<Language::Flow::Branch>();
  ASSERT(first && first->get_alternate());
  auto second =
      first->get_alternate()->get_root().select<Language::Flow::Branch>();
  ASSERT(second && second->get_alternate());
  EXPECT(second->get_alternate()->get_root().is<Language::Flow::Block>());
  EXPECT_NOT(first->reaches_next_statement());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(BranchTests, while_branch) {
  static constexpr View::Bytes source =
      "// While control.\n"
      "dialect : Library;\n"
      "public repeat : func = [] -> [] {\n"
      "  while true : continue;\n"
      "  return;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "repeat"_view);
  ASSERT(function && function->get_body());
  auto statements = function->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(2));
  const auto& loop = static_cast<const Language::Flow::Branch&>(
      statements.get_data()[0].get_root());
  const auto& control = static_cast<const Language::Flow::LoopControl&>(
      loop.get_body().get_statements().get_data()[0].get_root());
  EXPECT(&control.get_target() == &loop);
  EXPECT(loop.reaches_next_statement());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(BranchTests, incomplete_paths) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Missing alternate.\ndialect : Library; private invalid : func = [.flag : Bool] -> U64 { if flag { return 1; } }"_view,
    "// Falling alternate.\ndialect : Library; private invalid : func = [.flag : Bool] -> U64 { if flag { return 1; } else {} }"_view,
    "// While may not execute.\ndialect : Library; private invalid : func = [.flag : Bool] -> U64 { while flag { return 1; } }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(BranchTests, flag_condition) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Empty condition.\ndialect : Library; private invalid : func = [] -> [] { if () {} return; }"_view,
    "// Numeric condition.\ndialect : Library; private invalid : func = [] -> [] { if 1 {} return; }"_view,
    "// Later Flag.\ndialect : Library; private invalid : func = [] -> [] { while (1, true) {} return; }"_view,
    "// Type is not a value.\ndialect : Library; private invalid : func = [] -> [] { if Bool {} return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(BranchTests, malformed_branch) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Missing body.\ndialect : Library; private invalid : func = [] -> [] { if true return; }"_view,
    "// Missing alternate body.\ndialect : Library; private invalid : func = [] -> [] { if true {} else return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}
