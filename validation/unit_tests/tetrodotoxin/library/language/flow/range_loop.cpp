// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/range_loop.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/flow/loop_control.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/contiguous.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "tetrodotoxin/source/addressable.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness RangeLoopTests = {
  .name = "Tetrodotoxin::Library::Language::Flow::RangeLoop"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "RangeLoopTest"_view, "range_loop.ttx"_view, source);
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
         retains_library_source(workspace, "RangeLoopTest"_view);
}

static auto get_binding(const Language::Flow::RangeLoop& loop, Count index)
    -> Option<const Tetrodotoxin::Source::Addressable&> {
  auto entry = loop.get_bindings().get_abstract(index);
  return entry ? entry->select<Tetrodotoxin::Source::Addressable>()
               : Option<const Tetrodotoxin::Source::Addressable&>();
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, binding_identity) {
  static constexpr View::Bytes source =
      "// Range loop graph.\n"
      "dialect : Library;\n"
      "public sum : func = [] -> U64 {\n"
      "  state initial : U64 = 7;\n"
      "  state total : U64 = 0;\n"
      "  for [.entry : U64] in 0...3 {\n"
      "    state copy : U64 = entry;\n"
      "    total += copy;\n"
      "  }\n"
      "  return total;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "sum"_view);
  ASSERT(function && function->get_body());
  auto statements = function->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(4));
  ASSERT(statements.get_data()[0].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[1].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[2].get_root().is<Language::Flow::RangeLoop>());
  ASSERT(statements.get_data()[3].get_root().is<Language::Flow::Return>());

  const auto& loop = static_cast<const Language::Flow::RangeLoop&>(
      statements.get_data()[2].get_root());
  auto binding = get_binding(loop, 0);
  ASSERT(binding);
  EXPECT_TEXT(binding->get_name(), "entry"_view);
  EXPECT(binding->get_type().is<Language::Model::Types::Unsigned>());
  const Abstract& range_type = loop.get_input().get_type().resolve();
  ASSERT(range_type.is<Language::Types::Range>());
  EXPECT(
      &static_cast<const Language::Types::Range&>(range_type)
           .get_element_type() == &binding->get_type());
  EXPECT(&loop.resolve_concept("entry"_view) == &*binding);
  EXPECT(&loop.get_body().resolve_concept("entry"_view) == &*binding);
  EXPECT(
      &function->get_body()->resolve_concept("entry"_view) ==
      &Unknown::get_unknown());
  EXPECT(
      &loop.get_body().resolve_concept("total"_view) ==
      &statements.get_data()[1].get_root());
  EXPECT_TEXT(
      loop.get_anchor().get_span().caculate_text(source),
      "for [.entry : U64] in 0...3 {\n"
      "    state copy : U64 = entry;\n"
      "    total += copy;\n"
      "  }"_view);

  const Abstract& retained = statements.get_data()[2].get_root();
  Perimortem::Memory::Allocator::Arena transaction;
  Tokenizer tokenizer(transaction, source, "range_loop.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph->link(cursor));
  ASSERT(monograph->finalize(cursor));
  EXPECT(
      &function->get_body()->get_statements().get_data()[2].get_root() ==
      &retained);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, matching_range) {
  static constexpr Static::Vector<View::Bytes, 5> sources = {{
    "// Different integer Type.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : S64] in 0...3 {} return; }"_view,
    "// Not a Range.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : U64] in 3 {} return; }"_view,
    "// Multiple Range values.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : U64] in (0...3, 4...6) {} return; }"_view,
    "// Empty binding Type.\ndialect : Library; private Empty : struct {} private invalid : func = [] -> [] { for [.entry : Empty] in 0...3 {} return; }"_view,
    "// Shadowed binding.\ndialect : Library; private invalid : func = [] -> [] { state entry : U64 = 0; for [.entry : U64] in 0...3 {} return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, iterable_ranges) {
  static constexpr View::Bytes source =
      "// Contiguous loop inputs.\n"
      "dialect : Library;\n"
      "public scan : func = [] -> [] {\n"
      "  state writable : Access[U64];\n"
      "  state readonly : View[U64];\n"
      "  for [.entry : U64] in writable {}\n"
      "  for [.entry : U64] in readonly {}\n"
      "  return;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "scan"_view);
  ASSERT(function && function->get_body());
  auto statements = function->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(5));
  for (Count index = 2; index < 4; index++) {
    ASSERT(statements.get_data()[index]
               .get_root()
               .is<Language::Flow::RangeLoop>());
    const auto& loop = static_cast<const Language::Flow::RangeLoop&>(
        statements.get_data()[index].get_root());
    auto binding = get_binding(loop, 0);
    ASSERT(binding);
    const Abstract& input_type = loop.get_input().get_type().resolve();
    ASSERT(input_type.is<Language::Types::Contiguous>());
    EXPECT(
        &static_cast<const Language::Types::Contiguous&>(input_type)
             .get_element_type() == &binding->get_type());
  }
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, scoped_read_only) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Immutable binding.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : U64] in 0...3 { entry = 1; } return; }"_view,
    "// Leaked binding.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : U64] in 0...3 {} state copy : U64 = entry; return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, uncovered_result) {
  static constexpr View::Bytes source =
      "// Range may be empty.\n"
      "dialect : Library;\n"
      "private invalid : func = [] -> U64 {\n"
      "  for [.entry : U64] in 0...0 { return entry; }\n"
      "}"_view;
  EXPECT(rejects_link(source));
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, loop_control) {
  static constexpr View::Bytes source =
      "// Range control.\n"
      "dialect : Library;\n"
      "public scan : func = [] -> [] {\n"
      "  for [.entry : U64] in 0...2 : break;\n"
      "  return;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "scan"_view);
  ASSERT(function && function->get_body());
  const auto& loop = static_cast<const Language::Flow::RangeLoop&>(
      function->get_body()->get_statements().get_data()[0].get_root());
  const auto& control = static_cast<const Language::Flow::LoopControl&>(
      loop.get_body().get_statements().get_data()[0].get_root());
  EXPECT(control.get_kind() == Language::Flow::LoopControl::Kind::Break);
  EXPECT(&control.get_target() == &loop);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(RangeLoopTests, malformed_binding) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Bare binding.\ndialect : Library; private invalid : func = [] -> [] { for .entry : U64 in 0...3 {} return; }"_view,
    "// Empty binding.\ndialect : Library; private invalid : func = [] -> [] { for [] in 0...3 {} return; }"_view,
    "// Positional binding.\ndialect : Library; private invalid : func = [] -> [] { for [U64] in 0...3 {} return; }"_view,
    "// Missing body.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : U64] in 0...3 return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }

  EXPECT(rejects_link(
      "// Multiple bindings.\ndialect : Library; private invalid : func = [] -> [] { for [.left : U64, .right : U64] in 0...3 {} return; }"_view));
}
