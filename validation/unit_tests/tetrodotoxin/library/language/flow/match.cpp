// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/match.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/flow/branch.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/flow/loop_control.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
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

static Harness MatchTests = {
  .name = "Tetrodotoxin::Library::Language::Flow::Match"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "MatchTest"_view, "match.ttx"_view, source);
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
         retains_library_source(workspace, "MatchTest"_view);
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

PERIMORTEM_UNIT_TEST(MatchTests, flag_coverage) {
  static constexpr View::Bytes source =
      "// Complete match graph.\n"
      "dialect : Library;\n"
      "public choose : func = [.selector : Bool] -> U64 {\n"
      "  state outer : U64 = 9;\n"
      "  match selector {\n"
      "    case false : return outer;\n"
      "    case true {\n"
      "      state inner : U64 = 4;\n"
      "      return inner;\n"
      "    }\n"
      "  }\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "choose"_view);
  ASSERT(function && function->get_body());
  auto statements = function->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(2));
  const Abstract& outer = statements.get_data()[0].get_root();
  const auto& match = static_cast<const Language::Flow::Match&>(
      statements.get_data()[1].get_root());
  auto parameter = function->get_parameters().get_abstract(0);
  ASSERT(parameter);
  EXPECT(&match.get_input().get_result() == &*parameter);
  ASSERT_EQ(match.get_case_count(), Count(2));
  auto first = match.get_case_constant(0);
  auto second = match.get_case_constant(1);
  ASSERT(first && second);
  ASSERT(first->is<Language::Constants::Flag>());
  ASSERT(second->is<Language::Constants::Flag>());
  EXPECT_NOT(static_cast<const Language::Constants::Flag&>(*first).get_value());
  EXPECT(static_cast<const Language::Constants::Flag&>(*second).get_value());
  EXPECT_NOT(*first == *second);
  ASSERT(match.get_case_body(0));
  ASSERT(match.get_case_body(1));
  EXPECT(&match.get_case_body(0)->resolve_concept("outer"_view) == &outer);
  EXPECT(&match.get_case_body(1)->resolve_concept("outer"_view) == &outer);
  EXPECT(match.get_case_body(1)
             ->resolve_concept("inner"_view)
             .is<Language::Flow::Local>());
  EXPECT_NOT(match.get_default());
  EXPECT_NOT(match.reaches_next_statement());
  EXPECT_TEXT(
      match.get_anchor().get_span().caculate_text(source),
      "match selector {\n"
      "    case false : return outer;\n"
      "    case true {\n"
      "      state inner : U64 = 4;\n"
      "      return inner;\n"
      "    }\n"
      "  }"_view);

  const Language::Model::Pack& retained_input = match.get_input();
  const Language::Constant& retained_case = *second;
  Perimortem::Memory::Allocator::Arena transaction;
  Tokenizer tokenizer(transaction, source, "match.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph->link(cursor));
  ASSERT(monograph->finalize(cursor));
  EXPECT(&match.get_input() == &retained_input);
  EXPECT(&*match.get_case_constant(1) == &retained_case);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(MatchTests, folded_default) {
  static constexpr View::Bytes source =
      "// Folded case.\n"
      "dialect : Library;\n"
      "public choose : func = [] -> U64 {\n"
      "  match 2 {\n"
      "    case 1 + 1 : return 7;\n"
      "    case _ : return 9;\n"
      "  }\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "choose"_view);
  ASSERT(function && function->get_body());
  const auto& match = static_cast<const Language::Flow::Match&>(
      function->get_body()->get_statements().get_data()[0].get_root());
  auto folded = match.get_case_constant(0);
  ASSERT(folded && folded->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*folded).get_value(),
      U64(2));
  ASSERT(match.get_default());
  EXPECT_NOT(match.reaches_next_statement());

  Option<Language::Model::Pack&> input_folded;
  Language::Expression::fold(
      const_cast<Language::Model::Pack&>(match.get_input()))
      .visit(
          [&](const Option<Language::Model::Pack&>& selected) {
            input_folded = selected;
          },
          [](const Language::Expression::Error&) {});
  Bool first_case_selected = False;
  if (input_folded) {
    auto constant = input_folded->select_identity<Language::Constant>();
    first_case_selected = Bool(constant && *constant == *folded);
  }
  EXPECT(first_case_selected);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(MatchTests, nested_loop) {
  static constexpr View::Bytes source =
      "// Match loop control.\n"
      "dialect : Library;\n"
      "public run : func = [] -> [] {\n"
      "  while true {\n"
      "    match true {\n"
      "      case false : continue;\n"
      "      case true : break;\n"
      "    }\n"
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
  const auto& loop = static_cast<const Language::Flow::Branch&>(
      function->get_body()->get_statements().get_data()[0].get_root());
  const auto& match = static_cast<const Language::Flow::Match&>(
      loop.get_body().get_statements().get_data()[0].get_root());
  ASSERT(match.get_case_body(0));
  ASSERT(match.get_case_body(1));
  const auto& continued = static_cast<const Language::Flow::LoopControl&>(
      match.get_case_body(0)->get_statements().get_data()[0].get_root());
  const auto& broken = static_cast<const Language::Flow::LoopControl&>(
      match.get_case_body(1)->get_statements().get_data()[0].get_root());
  EXPECT(&continued.get_target() == &loop);
  EXPECT(&broken.get_target() == &loop);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(MatchTests, option_patterns) {
  static constexpr View::Bytes source =
      "// Option match graph.\n"
      "dialect : Library;\n"
      "public Maybe : alias = Option[U64];\n"
      "public choose : func = [.value : Maybe] -> U64 {\n"
      "  match value {\n"
      "    case item : return item;\n"
      "    case _ : return 0;\n"
      "  }\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto choose = find_function(monograph->get_source(), "choose"_view);
  ASSERT(choose && choose->get_body());
  const auto& first = static_cast<const Language::Flow::Match&>(
      choose->get_body()->get_statements().get_data()[0].get_root());

  ASSERT_EQ(first.get_case_count(), Count(1));
  ASSERT(first.get_case_kind(0));
  EXPECT(*first.get_case_kind(0) == Language::Flow::Match::CaseKind::Value);

  auto payload = first.get_case_payload(0);
  ASSERT(payload);
  EXPECT(&payload->get_type() == &monograph->resolve_concept("U64"_view));
  ASSERT(first.get_case_body(0));
  ASSERT(first.get_default());
  EXPECT(&first.get_case_body(0)->resolve_concept("item"_view) == &*payload);
  EXPECT(
      &first.get_default()->resolve_concept("item"_view) ==
      &Unknown::get_unknown());
  EXPECT_NOT(first.get_case_constant(0));
  EXPECT_NOT(first.reaches_next_statement());

  const Language::Model::Pack& retained_input = first.get_input();
  const Tetrodotoxin::Source::Addressable& retained_payload = *payload;
  Perimortem::Memory::Allocator::Arena transaction;
  Tokenizer tokenizer(transaction, source, "match.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph->link(cursor));
  ASSERT(monograph->finalize(cursor));
  EXPECT(&first.get_input() == &retained_input);
  EXPECT(&*first.get_case_payload(0) == &retained_payload);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(MatchTests, rejects_constructors) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Empty constructor.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> [] { match value { case some() {} case _ {} } return; }"_view,
    "// Payload constructor.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> [] { match value { case some(item) {} case _ {} } return; }"_view,
    "// Missing close.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> [] { match value { case some(item {} case _ {} } return; }"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_interpretation(sources[index]));
  }
}

PERIMORTEM_UNIT_TEST(MatchTests, invalid_option_cases) {
  static constexpr Static::Vector<View::Bytes, 6> sources = {{
    "// Missing absent case.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> [] { match value { case item {} } return; }"_view,
    "// Missing value case.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> [] { match value { case _ {} } return; }"_view,
    "// Duplicate value case.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> [] { match value { case first {} case second {} case _ {} } return; }"_view,
    "// Constant value case.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> [] { match value { case 7 {} case _ {} } return; }"_view,
    "// Empty element Type.\ndialect : Library; public Empty : struct {} private invalid : func = [.value : Option[Empty]] -> [] { return; }"_view,
    "// Shadowed payload.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> [] { match value { case value {} case _ {} } return; }"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_link(sources[index]));
  }
}

PERIMORTEM_UNIT_TEST(MatchTests, rejects_packs) {
  static constexpr Static::Vector<View::Bytes, 6> sources = {{
    "// Empty input.\ndialect : Library; private invalid : func = [] -> [] { match () {} return; }"_view,
    "// Named input.\ndialect : Library; private invalid : func = [] -> [] { match (.value = true) {} return; }"_view,
    "// Composed input.\ndialect : Library; private invalid : func = [] -> [] { match (true, false) {} return; }"_view,
    "// Empty case.\ndialect : Library; private invalid : func = [] -> [] { match true { case () {} } return; }"_view,
    "// Named case.\ndialect : Library; private invalid : func = [] -> [] { match true { case (.value = true) {} } return; }"_view,
    "// Composed case.\ndialect : Library; private invalid : func = [] -> [] { match true { case (true, false) {} } return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(MatchTests, malformed_default) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Nonfinal default.\ndialect : Library; private invalid : func = [] -> [] { match true { case _ {} case true {} } return; }"_view,
    "// Duplicate default.\ndialect : Library; private invalid : func = [] -> [] { match true { case _ {} case _ {} } return; }"_view,
    "// Missing Block.\ndialect : Library; private invalid : func = [] -> [] { match true { case true return; } return; }"_view,
    "// Missing case.\ndialect : Library; private invalid : func = [] -> [] { match true { true : {} } return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(MatchTests, invalid_cases) {
  static constexpr Static::Vector<View::Bytes, 5> sources = {{
    "// Duplicate Constant.\ndialect : Library; private invalid : func = [] -> [] { match 1 { case 1 {} case 0 + 1 {} } return; }"_view,
    "// Dynamic case.\ndialect : Library; private invalid : func = [.value : U64] -> [] { match value { case value {} case _ {} } return; }"_view,
    "// Different Type.\ndialect : Library; private invalid : func = [] -> [] { match 1 { case -1 {} case _ {} } return; }"_view,
    "// Incomplete coverage.\ndialect : Library; private invalid : func = [.value : Bool] -> U64 { match value { case true { return 1; } } }"_view,
    "// Statement after coverage.\ndialect : Library; private invalid : func = [] -> [] { match true { case false { return; } case true { return; } } return; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}
