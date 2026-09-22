// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/return.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness ReturnTests = {
  .name = "Tetrodotoxin::Library::Language::Flow::Return"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "ReturnTest"_view, "return.ttx"_view, source);
  BAIL_IF(!interpreted || !interpreted->is<Language::Monograph>());
  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  return !interpret(workspace, errors, source) && !errors.is_empty();
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

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  for (auto binding = composite.get_callables().begin();
       binding != composite.get_callables().end(); ++binding) {
    const Abstract& candidate = (*binding).get();
    if (candidate.get_name() == name && candidate.is<Language::Function>()) {
      return static_cast<const Language::Function&>(candidate);
    }
  }

  return {};
}

static auto find_return(const Language::Function& function)
    -> Option<const Language::Flow::Return&> {
  auto body = function.get_body();
  BAIL_IF(!body);
  for (const Language::Statement& statement : body->get_statements()) {
    auto returned = statement.get_root().select<Language::Flow::Return>();
    if (returned) {
      return *returned;
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(ReturnTests, layout_fitting) {
  static constexpr View::Bytes source =
      "// Return Layout flow.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public state number : U64; public state flag : Bool;\n"
      "}\n"
      "public Flow : struct {\n"
      "  public bare_result : func = [] -> [] { return; }\n"
      "  public explicit_empty : func = [] -> [] { return (); }\n"
      "  public fallthrough : func = [] -> [] {}\n"
      "  public bare_empty : func = [] -> [] { return; }\n"
      "  public scalar : func = [] -> Bool { return true; }\n"
      "  public grouped_scalar : func = [] -> Bool { return (true); }\n"
      "  public grouped_pair : func = [.packet : Packet] -> "
      "[U64, Bool] {\n"
      "    return (packet.number, packet.flag);\n"
      "  }\n"
      "  public named_pair : func = [.packet : Packet] -> "
      "[.number : U64, .flag : Bool] {\n"
      "    return (.flag = packet.flag, .number = packet.number);\n"
      "  }\n"
      "  public pair : func = [.packet : Packet] -> [U64, Bool] {\n"
      "    return packet.[number, flag];\n"
      "  }\n"
      "  public called : func = [.packet : Packet] -> [U64, Bool] {\n"
      "    return Flow -> pair(packet);\n"
      "  }\n"
      "  public empty_swizzle : func = [.packet : Packet] -> [] {\n"
      "    return packet.[];\n"
      "  }\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& flow_identity = monograph->resolve_concept("Flow"_view);
  ASSERT(flow_identity.is<Language::Types::Structure>());
  const auto& flow =
      static_cast<const Language::Types::Structure&>(flow_identity);
  auto bare_result = find_function(flow, "bare_result"_view);
  auto explicit_empty = find_function(flow, "explicit_empty"_view);
  auto fallthrough = find_function(flow, "fallthrough"_view);
  auto bare_empty = find_function(flow, "bare_empty"_view);
  auto scalar = find_function(flow, "scalar"_view);
  auto grouped_scalar = find_function(flow, "grouped_scalar"_view);
  auto grouped_pair = find_function(flow, "grouped_pair"_view);
  auto named_pair = find_function(flow, "named_pair"_view);
  auto pair = find_function(flow, "pair"_view);
  auto called = find_function(flow, "called"_view);
  auto empty_swizzle = find_function(flow, "empty_swizzle"_view);
  ASSERT(bare_result && explicit_empty && fallthrough && bare_empty && scalar);
  ASSERT(grouped_scalar && grouped_pair && named_pair && pair && called);
  ASSERT(empty_swizzle);

  auto bare_return = find_return(*bare_result);
  auto explicit_return = find_return(*explicit_empty);
  auto empty_return = find_return(*bare_empty);
  auto scalar_return = find_return(*scalar);
  auto grouped_scalar_return = find_return(*grouped_scalar);
  auto grouped_pair_return = find_return(*grouped_pair);
  auto named_pair_return = find_return(*named_pair);
  auto pair_return = find_return(*pair);
  auto called_return = find_return(*called);
  auto swizzle_return = find_return(*empty_swizzle);
  ASSERT(bare_return && explicit_return && empty_return && scalar_return);
  ASSERT(grouped_scalar_return && grouped_pair_return && named_pair_return);
  ASSERT(pair_return && called_return && swizzle_return);
  EXPECT(
      bare_return->get_anchor().get_span().caculate_text(source) ==
      "return;"_view);
  EXPECT(bare_result->get_results().is_empty());
  EXPECT(explicit_empty->get_results().is_empty());
  EXPECT(bare_empty->get_results().is_empty());
  EXPECT(bare_result->get_results().fits(explicit_empty->get_results()));
  EXPECT(explicit_empty->get_results().fits(bare_empty->get_results()));
  ASSERT(fallthrough->get_body());
  EXPECT(fallthrough->get_body()->get_statements().is_empty());
  EXPECT_EQ(scalar->get_results().get_size(), Count(1));
  EXPECT_EQ(grouped_scalar->get_results().get_size(), Count(1));
  EXPECT_EQ(grouped_pair->get_results().get_size(), Count(2));
  EXPECT_EQ(named_pair->get_results().get_size(), Count(2));
  EXPECT_EQ(pair->get_results().get_size(), Count(2));
  EXPECT_EQ(called->get_results().get_size(), Count(2));
  EXPECT(empty_swizzle->get_results().is_empty());

  Perimortem::Memory::Allocator::Arena transaction;
  Tokenizer tokenizer(transaction, source, "return.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph->link(cursor));
  ASSERT(monograph->finalize(cursor));
  EXPECT(&*find_return(*scalar) == &*scalar_return);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ReturnTests, incompatible_flow) {
  static constexpr Static::Vector<View::Bytes, 7> sources = {{
    "// Missing return.\ndialect : Library; private invalid : func = [] -> Bool {}"_view,
    "// Bare nonempty return.\ndialect : Library; private invalid : func = [] -> Bool { return; }"_view,
    "// Value in empty return.\ndialect : Library; private invalid : func = [] -> [] { return true; }"_view,
    "// Scalar mismatch.\ndialect : Library; private invalid : func = [] -> Bool { return 1; }"_view,
    "// Explicit empty mismatch.\ndialect : Library; private invalid : func = [] -> Bool { return (); }"_view,
    "// Positional Pack mismatch.\ndialect : Library; private invalid : func = [] -> [U64, Bool] { return (true, 1); }"_view,
    "// Named Pack mismatch.\ndialect : Library; private invalid : func = [] -> [.left : Bool] { return (.right = true); }"_view,
  }};
  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(ReturnTests, syntax_rollback) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Unreachable statement.\ndialect : Library; private invalid : func = [] -> [] { return; Unknown -> call(); }"_view,
    "// Missing return terminator.\ndialect : Library; private invalid : func = [] -> [] { return }"_view,
    "// Missing Pack closing parenthesis.\ndialect : Library; private invalid : func = [] -> Bool { return (true; }"_view,
  }};
  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}
