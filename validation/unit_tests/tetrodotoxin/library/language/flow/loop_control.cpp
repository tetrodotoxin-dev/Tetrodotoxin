// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/loop_control.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/flow/branch.hpp"
#include "tetrodotoxin/library/language/flow/range_loop.hpp"
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

static Harness LoopControlTests = {
  .name = "Tetrodotoxin::Library::Language::Flow::LoopControl"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "LoopControlTest"_view, "loop_control.ttx"_view, source);
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

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  return !interpret(workspace, errors, source) && !errors.is_empty() &&
         retains_library_source(workspace, "LoopControlTest"_view);
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

PERIMORTEM_UNIT_TEST(LoopControlTests, nearest_loop) {
  static constexpr View::Bytes source =
      "// Loop control graph.\n"
      "dialect : Library;\n"
      "public run : func = [] -> [] {\n"
      "  while true {\n"
      "    if true { continue; }\n"
      "    break;\n"
      "  }\n"
      "  for [.entry : U64] in 0...2 {\n"
      "    if true { continue; }\n"
      "    while false { break; }\n"
      "    continue;\n"
      "  }\n"
      "  return;\n"
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
  ASSERT_EQ(statements.get_size(), Count(3));
  ASSERT(statements.get_data()[0].get_root().is<Language::Flow::Branch>());
  ASSERT(statements.get_data()[1].get_root().is<Language::Flow::RangeLoop>());

  const auto& while_loop = static_cast<const Language::Flow::Branch&>(
      statements.get_data()[0].get_root());
  auto while_body = while_loop.get_body().get_statements();
  ASSERT_EQ(while_body.get_size(), Count(2));
  const auto& conditional = static_cast<const Language::Flow::Branch&>(
      while_body.get_data()[0].get_root());
  const auto& nested_continue = static_cast<const Language::Flow::LoopControl&>(
      conditional.get_body().get_statements().get_data()[0].get_root());
  const auto& direct_break = static_cast<const Language::Flow::LoopControl&>(
      while_body.get_data()[1].get_root());
  EXPECT(
      nested_continue.get_kind() ==
      Language::Flow::LoopControl::Kind::Continue);
  EXPECT(direct_break.get_kind() == Language::Flow::LoopControl::Kind::Break);
  EXPECT(&nested_continue.get_target() == &while_loop);
  EXPECT(&direct_break.get_target() == &while_loop);

  const auto& range_loop = static_cast<const Language::Flow::RangeLoop&>(
      statements.get_data()[1].get_root());
  auto range_body = range_loop.get_body().get_statements();
  ASSERT_EQ(range_body.get_size(), Count(3));
  const auto& range_conditional = static_cast<const Language::Flow::Branch&>(
      range_body.get_data()[0].get_root());
  const auto& range_continue = static_cast<const Language::Flow::LoopControl&>(
      range_conditional.get_body().get_statements().get_data()[0].get_root());
  const auto& inner_while = static_cast<const Language::Flow::Branch&>(
      range_body.get_data()[1].get_root());
  const auto& inner_break = static_cast<const Language::Flow::LoopControl&>(
      inner_while.get_body().get_statements().get_data()[0].get_root());
  const auto& final_continue = static_cast<const Language::Flow::LoopControl&>(
      range_body.get_data()[2].get_root());
  EXPECT(&range_continue.get_target() == &range_loop);
  EXPECT(&inner_break.get_target() == &inner_while);
  EXPECT(&final_continue.get_target() == &range_loop);
  EXPECT_TEXT(
      inner_break.get_anchor().get_span().caculate_text(source), "break;"_view);

  const Abstract& retained = range_body.get_data()[2].get_root();
  Perimortem::Memory::Allocator::Arena transaction;
  Tokenizer tokenizer(transaction, source, "loop_control.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph->link(cursor));
  ASSERT(monograph->finalize(cursor));
  EXPECT(
      &range_loop.get_body().get_statements().get_data()[2].get_root() ==
      &retained);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LoopControlTests, outside_loop) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Bare break.\ndialect : Library; private invalid : func = [] -> [] { break; }"_view,
    "// Bare continue.\ndialect : Library; private invalid : func = [] -> [] { continue; }"_view,
    "// Nested branch.\ndialect : Library; private invalid : func = [] -> [] { if true { break; } return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(LoopControlTests, must_end_its_block) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Form after break.\ndialect : Library; private invalid : func = [] -> [] { while true { break; return; } return; }"_view,
    "// Form after continue.\ndialect : Library; private invalid : func = [] -> [] { for [.entry : U64] in 0...1 { continue; return; } return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(LoopControlTests, uncovered_result) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// While may break.\ndialect : Library; private invalid : func = [] -> U64 { while true { break; } }"_view,
    "// Range may continue zero times.\ndialect : Library; private invalid : func = [] -> U64 { for [.entry : U64] in 0...0 { continue; } }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}
