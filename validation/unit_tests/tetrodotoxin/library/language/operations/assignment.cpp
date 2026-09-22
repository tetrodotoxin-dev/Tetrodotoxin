// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/assignment.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/operations/add_assignment.hpp"
#include "tetrodotoxin/library/language/operations/subtract.hpp"
#include "tetrodotoxin/library/language/operations/subtract_assignment.hpp"
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

static Harness AssignmentTests = {
  .name = "Tetrodotoxin::Library::Language::Operations::Assignment"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "AssignmentTest"_view, "assignment.ttx"_view, source);
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

static auto rejects_source(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  return !interpret(workspace, errors, source) && !errors.is_empty() &&
         retains_library_source(workspace, "AssignmentTest"_view);
}

PERIMORTEM_UNIT_TEST(AssignmentTests, lowest_precedence) {
  static constexpr View::Bytes source =
      "// Assignment graph.\n"
      "dialect : Library;\n"
      "public Data : struct {\n"
      "  public value : U64 = 0;\n"
      "  expose state guarded : U64 = 0;\n"
      "  public mutate : func = [self] -> [] {\n"
      "    self.guarded += 1;\n"
      "    return;\n"
      "  }\n"
      "}\n"
      "private global : U64 = 0;\n"
      "public run : func = [] -> [] {\n"
      "  state local : U64 = 1;\n"
      "  state access : Access[U64];\n"
      "  Data.value = local + 2 * 3;\n"
      "  global = Data.value;\n"
      "  local += 3;\n"
      "  local -= 1;\n"
      "  access[0] = local;\n"
      "  return;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto run = find_function(monograph->get_source(), "run"_view);
  ASSERT(run && run->get_body());
  auto statements = run->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(8));

  for (Count index = 0; index < 5; index++) {
    const Abstract& semantic = statements.get_data()[index + 2].get_root();
    ASSERT(semantic.is<Language::Expression>());
    const auto& expression = static_cast<const Language::Expression&>(semantic);
    EXPECT(expression.is_complete());
    EXPECT(expression.get_layout().is_empty());
  }

  ASSERT(statements.get_data()[2]
             .get_root()
             .is<Language::Operations::Assignment>());
  ASSERT(statements.get_data()[3]
             .get_root()
             .is<Language::Operations::Assignment>());
  ASSERT(statements.get_data()[6]
             .get_root()
             .is<Language::Operations::Assignment>());
  const auto& precedence = static_cast<const Language::Operations::Assignment&>(
      statements.get_data()[2].get_root());
  EXPECT(precedence.get_target().resolve().is<Language::Expression>());
  EXPECT(precedence.get_source().is_complete());
  EXPECT(precedence.get_source().is_identity<Language::Operations::Add>());
  ASSERT(precedence.get_anchor());
  EXPECT_TEXT(
      precedence.get_anchor()->get_span().caculate_text(source),
      "Data.value = local + 2 * 3"_view);

  ASSERT(statements.get_data()[4]
             .get_root()
             .is<Language::Operations::AddAssignment>());
  const auto& addition =
      static_cast<const Language::Operations::AddAssignment&>(
          statements.get_data()[4].get_root());
  ASSERT(statements.get_data()[5]
             .get_root()
             .is<Language::Operations::SubtractAssignment>());
  const auto& subtraction =
      static_cast<const Language::Operations::SubtractAssignment&>(
          statements.get_data()[5].get_root());
  EXPECT(addition.get_target().resolve().is<Language::Expression>());
  EXPECT(subtraction.get_target().resolve().is<Language::Expression>());
  EXPECT_NOT(addition.get_right().is_identity<Language::Operations::Add>());
  EXPECT_NOT(
      subtraction.get_right().is_identity<Language::Operations::Subtract>());
  ASSERT(addition.get_right().get_anchor());
  ASSERT(subtraction.get_right().get_anchor());
  EXPECT_TEXT(
      addition.get_right().get_anchor()->get_span().caculate_text(source),
      "3"_view);
  EXPECT_TEXT(
      subtraction.get_right().get_anchor()->get_span().caculate_text(source),
      "1"_view);

  const Abstract& retained = statements.get_data()[2].get_root();
  Perimortem::Memory::Allocator::Arena transaction;
  Tokenizer tokenizer(transaction, source, "assignment.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph->link(cursor));
  ASSERT(monograph->finalize(cursor));
  EXPECT(
      &run->get_body()->get_statements().get_data()[2].get_root() == &retained);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AssignmentTests, pack_fitting) {
  static constexpr View::Bytes source =
      "// Assignment Pack.\n"
      "dialect : Library;\n"
      "public Pair : struct {\n"
      "  public state number : U64;\n"
      "  public state flag : Bool;\n"
      "}\n"
      "public run : func = [] -> Pair {\n"
      "  state pair : Pair;\n"
      "  pair = (1, true);\n"
      "  return pair;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  ASSERT(interpret(workspace, errors, source));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AssignmentTests, immutable_targets) {
  static constexpr View::Bytes writable =
      "// Public state write.\n"
      "dialect : Library;\n"
      "public Data : struct { public state value : U64; }\n"
      "private data : Data;\n"
      "private write : func = [] -> [] { data.value = 1; return; }"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  ASSERT(interpret(workspace, errors, writable));
  EXPECT(errors.is_empty());

  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Const Local.\ndialect : Library; private invalid : func = [] -> [] { const value : U64 = 1; value = 2; return; }"_view,
    "// Const Field.\ndialect : Library; private const value : U64 = 1; private invalid : func = [] -> [] { value = 2; return; }"_view,
    "// Parameter target.\ndialect : Library; private invalid : func = [.value : U64] -> [] { value = 2; return; }"_view,
    "// External state.\ndialect : Library; public Data : struct { expose state value : U64 = 0; } private data : Data; private invalid : func = [] -> [] { data.value = 2; return; }"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_source(sources[index]));
  }
}

PERIMORTEM_UNIT_TEST(AssignmentTests, invalid_assignment) {
  static constexpr Static::Vector<View::Bytes, 7> sources = {{
    "// Plain mismatch.\ndialect : Library; private invalid : func = [] -> [] { state value : Bool = false; value = 1; return; }"_view,
    "// Compound mismatch.\ndialect : Library; private invalid : func = [] -> [] { state value : U64 = 0; value += -1; return; }"_view,
    "// Compound nonnumeric.\ndialect : Library; private invalid : func = [] -> [] { state value : Bool = false; value += true; return; }"_view,
    "// Pack mismatch.\ndialect : Library; private invalid : func = [] -> [] { state value : U64 = 0; value = (1, 2); return; }"_view,
    "// Computed target.\ndialect : Library; private invalid : func = [] -> [] { 1 + 2 = 3; return; }"_view,
    "// Missing source.\ndialect : Library; private invalid : func = [] -> [] { state value : U64 = 0; value = ; return; }"_view,
    "// Type target.\ndialect : Library; private invalid : func = [] -> [] { Bool = true; return; }"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_source(sources[index]));
  }
}
