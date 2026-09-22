// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/block.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness BlockTests = {
  .name = "Tetrodotoxin::Library::Language::Flow::Block"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "BlockTest"_view, "block.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
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

PERIMORTEM_UNIT_TEST(BlockTests, scope_order) {
  static constexpr View::Bytes source =
      "// Block owner.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public touch : func = [] -> [] : return;\n"
      "  public empty : func = [] -> [] {}\n"
      "  public body : func = [.input : U64] -> U64 {\n"
      "    (Packet -> touch());\n"
      "    return input;\n"
      "  }\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& packet_identity =
      monograph->get_source().resolve_concept("Packet"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  auto empty = find_function(packet, "empty"_view);
  auto body = find_function(packet, "body"_view);
  auto touch = find_function(packet, "touch"_view);
  ASSERT(empty && empty->get_body());
  ASSERT(body && body->get_body());
  ASSERT(touch && touch->get_body());

  const Language::Flow::Block& empty_block = *empty->get_body();
  const Language::Flow::Block& populated = *body->get_body();
  const Language::Flow::Block& single = *touch->get_body();
  EXPECT(empty_block.get_statements().is_empty());
  EXPECT(
      empty_block.get_anchor().get_span().caculate_text(source) == "{}"_view);
  EXPECT(
      populated.get_anchor().get_span().caculate_text(source) ==
      "{\n    (Packet -> touch());\n    return input;\n  }"_view);
  ASSERT_EQ(single.get_statements().get_size(), Count(1));
  EXPECT(single.get_statements()
             .get_data()[0]
             .get_root()
             .is<Language::Flow::Return>());
  EXPECT_TEXT(
      single.get_anchor().get_span().caculate_text(source), ": return;"_view);

  auto statements = populated.get_statements();
  ASSERT_EQ(statements.get_size(), Count(2));
  const Abstract& first = statements.get_data()[0].get_root();
  const Abstract& second = statements.get_data()[1].get_root();
  EXPECT(&first != &second);
  ASSERT(first.is<Language::Access::Call>());
  ASSERT(second.is<Language::Flow::Return>());
  const auto& invoked = static_cast<const Language::Access::Call&>(first);
  const auto& returned = static_cast<const Language::Flow::Return&>(second);
  EXPECT(invoked.get_callable());
  EXPECT_NOT(invoked.get_folded());
  EXPECT_TEXT(
      returned.get_anchor().get_span().caculate_text(source),
      "return input;"_view);

  auto parameter = body->get_parameters().get_abstract(0);
  ASSERT(parameter);
  EXPECT(&populated.resolve_concept("input"_view) == &*parameter);

  Perimortem::Memory::Allocator::Arena transaction;
  Tokenizer tokenizer(transaction, source, "block.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph->link(cursor));
  ASSERT(monograph->finalize(cursor));
  auto repeated = populated.get_statements();
  ASSERT_EQ(repeated.get_size(), Count(2));
  EXPECT(&repeated.get_data()[0].get_root() == &first);
  EXPECT(&repeated.get_data()[1].get_root() == &second);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(BlockTests, expressions) {
  static constexpr View::Bytes source =
      "// Free expressions.\n"
      "dialect : Library;\n"
      "public Packet : struct { public state value : Bool; }\n"
      "private packet : Packet;\n"
      "private run : func = [] -> [] {\n"
      "  true;\n"
      "  1 + 2;\n"
      "  packet.value;\n"
      "  Bool;\n"
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
  ASSERT_EQ(statements.get_size(), Count(5));
  for (Count index = 0; index < 4; index++) {
    EXPECT(statements.get_data()[index].get_pack());
  }
  EXPECT(statements.get_data()[4].get_root().is<Language::Flow::Return>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(BlockTests, nested_blocks) {
  static constexpr View::Bytes source =
      "// Nested Block.\n"
      "dialect : Library;\n"
      "private run : func = [] -> [] {\n"
      "  // Nested execution scope.\n"
      "  {\n"
      "    // Discarded value.\n"
      "    true;\n"
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

  auto run = find_function(monograph->get_source(), "run"_view);
  ASSERT(run && run->get_body());
  auto statements = run->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(2));
  const Language::Statement& nested_statement = statements.get_data()[0];
  auto nested = nested_statement.get_root().select<Language::Flow::Block>();
  ASSERT(nested);
  EXPECT_EQ(nested_statement.get_documentation().line_count(), Count(1));
  EXPECT_TEXT(
      nested_statement.get_documentation().get_line(0),
      "Nested execution scope."_view);

  auto nested_statements = nested->get_statements();
  ASSERT_EQ(nested_statements.get_size(), Count(1));
  EXPECT(nested_statements.get_data()[0].get_pack());
  EXPECT_EQ(
      nested_statements.get_data()[0].get_documentation().line_count(),
      Count(1));
  EXPECT_TEXT(
      nested_statements.get_data()[0].get_documentation().get_line(0),
      "Discarded value."_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(BlockTests, scope_rejection) {
  static constexpr View::Bytes source =
      "// Block rollback.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public body : func = [.input : U64] -> U64 {\n"
      "    return input;\n"
      "    input;\n"
      "  }\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  EXPECT_NOT(interpret(workspace, errors, source));
  EXPECT_NOT(errors.is_empty());
  EXPECT(retains_library_source(workspace, "BlockTest"_view));
}
