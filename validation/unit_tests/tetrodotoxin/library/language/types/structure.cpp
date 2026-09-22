// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

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

static auto has_diagnostic(const Errors& errors, View::Bytes fragment) -> Bool {
  Allocator::Arena rendered;
  for (Count index = 0; index < errors.get_size(); index++) {
    if (Algorithm::search(errors.render_message(rendered, index), fragment) !=
        Count(-1)) {
      return True;
    }
  }

  return False;
}

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "StructureTest"_view, "structure.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto parse_authored(
    Allocator::Arena& lexical,
    Dialect& dialect,
    Workspace& context,
    Errors& errors,
    View::Bytes source) -> Option<Language::Monograph&> {
  Tokenizer tokenizer(lexical, source, "structure.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  if (!cursor.get_code().is_comment()) {
    return {};
  }

  Token source_opening = cursor.current();
  const Tetrodotoxin::Source::Documentation& documentation =
      Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  Token dialect_declaration = cursor.current();
  if (Tetrodotoxin::Language::Parser::Dialect::parse(cursor) !=
      "Library"_view) {
    return {};
  }

  Anchor source_anchor = Anchor::create(
      dialect_declaration, Span(source_opening, cursor.peek(-1)));
  auto interpretation =
      dialect.interpret(cursor, documentation, source_anchor, context);
  if (!interpretation || !cursor.matches(Code::Type::Terminal) ||
      !interpretation->is<Language::Monograph>() || !errors.is_empty()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpretation);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return !monograph && !errors.is_empty();
}

static auto rejects_link(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto& dialect = get_library_dialect(*workspace_toolchain);
  Allocator::Arena lexical;
  auto monograph = parse_authored(lexical, dialect, workspace, errors, source);
  if (!monograph) {
    return False;
  }

  Allocator::Arena completion;
  Tokenizer tokenizer(completion, source, "structure.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  Bool linked = monograph->link(cursor);
  return !linked && !errors.is_empty();
}

static auto rejects_finalize(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto& dialect = get_library_dialect(*workspace_toolchain);
  Allocator::Arena lexical;
  auto monograph = parse_authored(lexical, dialect, workspace, errors, source);
  if (!monograph) {
    return False;
  }

  Allocator::Arena completion;
  Tokenizer tokenizer(completion, source, "structure.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  if (!monograph->link(cursor)) {
    return False;
  }

  Bool finalized = monograph->finalize(cursor);
  return !finalized && !errors.is_empty();
}

static Harness StructureTests = {
  .name = "Tetrodotoxin::Library::Language::Types::Structure"_view,
};

PERIMORTEM_UNIT_TEST(StructureTests, nested_type_aliases) {
  static constexpr View::Bytes source =
      "// Nested Alias source.\n"
      "dialect : Library;\n"
      "// Hidden Type documentation.\n"
      "private Hidden : struct { private state value : Bool; }\n"
      "public Packet : struct {\n"
      "  // Visible Alias documentation.\n"
      "  public Visible : alias = Hidden;\n"
      "  private Flag : alias = Bool;\n"
      "  public value : Visible;\n"
      "  private flag : Flag;\n"
      "}\n"
      "public Selected : alias = Packet::Visible;"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto& dialect = get_library_dialect(*workspace_toolchain);
  Allocator::Arena lexical;
  auto owner = parse_authored(lexical, dialect, workspace, errors, source);
  ASSERT(owner);
  auto& monograph = *owner;
  const auto& source_type = monograph.get_source();
  auto types = source_type.get_types();
  ASSERT(types != types.end());
  const Abstract& hidden = (*types).get();
  const Abstract& packet_identity = monograph.resolve_concept("Packet"_view);
  const Abstract& selected_identity =
      monograph.resolve_concept("Selected"_view);
  ASSERT(hidden.is<Language::Types::Structure>());
  ASSERT(packet_identity.is<Language::Types::Structure>());
  ASSERT(selected_identity.is<Alias>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  const Abstract& visible_identity = packet.resolve_concept("Visible"_view);
  ASSERT(visible_identity.is<Alias>());
  const auto& visible = static_cast<const Alias&>(visible_identity);

  Allocator::Arena completion;
  Tokenizer tokenizer(completion, source, "structure.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph.link(cursor));
  ASSERT(monograph.finalize(cursor));
  EXPECT(&visible.resolve() == &hidden);
  EXPECT_EQ(visible.get_documentation().line_count(), Count(2));
  EXPECT_TEXT(
      visible.get_documentation().get_line(0),
      "Visible Alias documentation."_view);
  EXPECT_TEXT(
      visible.get_documentation().get_line(1),
      "Hidden Type documentation."_view);
  EXPECT(&packet.resolve_concept("Flag"_view) == &Unknown::get_unknown());
  auto type_bindings = packet.get_types();
  ASSERT(type_bindings != type_bindings.end());
  ++type_bindings;
  ASSERT(type_bindings != type_bindings.end());
  ASSERT((*type_bindings).get().is<Alias>());
  EXPECT(
      &(*type_bindings).get().resolve() ==
      &monograph.resolve_concept("Bool"_view));

  EXPECT(&selected_identity.resolve() == &hidden);
  auto fields = packet.get_addressables();
  ASSERT(fields != fields.end());
  const auto& hidden_field =
      static_cast<const Language::Field&>((*fields).get());
  ++fields;
  ASSERT(fields != fields.end());
  const auto& flag_field = static_cast<const Language::Field&>((*fields).get());
  EXPECT(&hidden_field.get_type() == &hidden);
  EXPECT(&flag_field.get_type() == &monograph.resolve_concept("Bool"_view));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, contextual_routes) {
  static constexpr View::Bytes source =
      "// Context route test.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  private Hidden : struct { private state value : Bool; }\n"
      "  public Visible : alias = Hidden;\n"
      "  public Inner : struct {\n"
      "    public Leaf : struct { private state value : Bool; }\n"
      "  }\n"
      "  private state direct : Hidden;\n"
      "  public state redirected : Visible;\n"
      "  private state qualified : Outer::Inner::Leaf;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const auto& outer = static_cast<const Language::Types::Structure&>(
      monograph->resolve_concept("Outer"_view));
  EXPECT(&outer.resolve_concept("Hidden"_view) == &Unknown::get_unknown());
  const Abstract& visible = outer.resolve_concept("Visible"_view);
  ASSERT(visible.is<Alias>());
  EXPECT(&visible.resolve_concept("value"_view) == &None::get_none());

  const Abstract& inner = outer.resolve_concept("Inner"_view);
  ASSERT(inner.is<Language::Types::Structure>());
  const Abstract& leaf = inner.resolve_concept("Leaf"_view);
  ASSERT(leaf.is<Language::Types::Structure>());

  auto fields = outer.get_addressables();
  ASSERT(fields != fields.end());
  const auto& direct = static_cast<const Language::Field&>((*fields).get());
  ++fields;
  ASSERT(fields != fields.end());
  const auto& redirected = static_cast<const Language::Field&>((*fields).get());
  ++fields;
  ASSERT(fields != fields.end());
  const auto& qualified = static_cast<const Language::Field&>((*fields).get());
  EXPECT(&direct.get_type() == &visible.resolve());
  EXPECT(&redirected.get_type() == &visible.resolve());
  EXPECT(&qualified.get_type() == &leaf);
  EXPECT(errors.is_empty());

  static constexpr View::Bytes rejected =
      "// Qualified private route test.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  private Hidden : struct { private state value : Bool; }\n"
      "  private state invalid : Outer::Hidden;\n"
      "}"_view;
  EXPECT(rejects_link(rejected));
}

PERIMORTEM_UNIT_TEST(StructureTests, access_axes) {
  static constexpr View::Bytes source =
      "// Structure access test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public ordinary_public : Bool;\n"
      "  private ordinary_private : Bool;\n"
      "  public state state_public : Bool;\n"
      "  expose state state_exposed : Bool = false;\n"
      "  private state state_private : Bool = false;\n"
      "  public const const_public : Bool = false;\n"
      "  private const const_private : Bool = false;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& selected = monograph->resolve_concept("Packet"_view);
  ASSERT(selected.is<Language::Types::Structure>());
  const auto& packet = static_cast<const Language::Types::Structure&>(selected);
  auto fields = packet.get_addressables();

  auto field = fields.begin();
  ASSERT(field != fields.end());
  const auto& ordinary_public =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& ordinary_private =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& state_public =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& state_exposed =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& state_private =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& const_public =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& const_private =
      static_cast<const Language::Field&>((*field).get());
  EXPECT(ordinary_public.get_writability() == Language::Writability::Full);
  EXPECT(ordinary_private.get_writability() == Language::Writability::Full);
  EXPECT(state_public.get_writability() == Language::Writability::Internal);
  EXPECT(state_exposed.get_writability() == Language::Writability::Internal);
  EXPECT(state_private.get_writability() == Language::Writability::Internal);
  EXPECT(const_public.get_writability() == Language::Writability::Constant);
  EXPECT(const_private.get_writability() == Language::Writability::Constant);
  ASSERT_EQ(packet.get_layout().get_size(), Count(3));
  auto public_layout_field = packet.get_layout().get_abstract(0);
  auto exposed_layout_field = packet.get_layout().get_abstract(1);
  auto private_layout_field = packet.get_layout().get_abstract(2);
  ASSERT(public_layout_field);
  ASSERT(exposed_layout_field);
  ASSERT(private_layout_field);
  EXPECT(&*public_layout_field == &state_public);
  EXPECT(&*exposed_layout_field == &state_exposed);
  EXPECT(&*private_layout_field == &state_private);

  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, declaration_reorder) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "public First : struct { public state next : Second; }\n"
    "public Second : struct { public state value : U64; }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Second : struct { public state value : U64; }\n"
    "public First : struct { public state next : Second; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Tetrodotoxin::Library::Dialect workspace_toolchain_library;
    auto workspace_toolchain =
        Validation::create_library_toolchain(workspace_toolchain_library);
    Workspace workspace(*workspace_toolchain);
    Errors errors;
    auto monograph = interpret(workspace, errors, sources[i]);
    ASSERT(monograph);

    const Abstract& first = monograph->resolve_concept("First"_view);
    const Abstract& second = monograph->resolve_concept("Second"_view);
    ASSERT(first.is<Language::Types::Structure>());
    ASSERT(second.is<Language::Types::Structure>());
    const auto& first_structure =
        static_cast<const Language::Types::Structure&>(first);
    auto field = first_structure.get_layout().get_abstract(0);
    ASSERT(field);
    ASSERT(field->is<Addressable>());
    EXPECT(&static_cast<const Addressable&>(*field).get_type() == &second);
    EXPECT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, category_names) {
  static constexpr Static::Vector<View::Bytes, 3> accepted = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public state value : Bool; public value : func = [] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { private state storage : Bool; public value : func = [] -> [] {} public value : func = [self] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public state value : Bool; public inspect : func = [self, .value : Bool] -> Bool { return value; } }"_view,
  }};
  for (Count i = 0; i < accepted.get_size(); i++) {
    Tetrodotoxin::Library::Dialect workspace_toolchain_library;
    auto workspace_toolchain =
        Validation::create_library_toolchain(workspace_toolchain_library);
    Workspace workspace(*workspace_toolchain);
    Errors errors;
    auto monograph = interpret(workspace, errors, accepted[i]);
    ASSERT(monograph);
    EXPECT(errors.is_empty());
  }

  static constexpr View::Bytes duplicate_fields =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Packet : struct { public value : Bool; private value : Bool; }"_view;
  EXPECT(rejects_interpretation(duplicate_fields));

  static constexpr View::Bytes static_state_collision =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public state value : U8;\n"
      "  public value : U8;\n"
      "}\n"
      "public run : func = [] -> [] {\n"
      "  state later : U8;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  EXPECT(interpret(workspace, errors, static_state_collision));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, indexed_name_domains) {
  static constexpr View::Bytes source =
      "// Structure lookup test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public state value : Bool;\n"
      "  private state hidden : Bool;\n"
      "  public value : func = [] -> [] {}\n"
      "  public invoke : func = [] -> [] {}\n"
      "  public invoke : func = [self] -> [] {}\n"
      "  public Visible : struct {}\n"
      "  private Hidden : struct {}\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& packet_identity = monograph->resolve_concept("Packet"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  const Abstract& field =
      packet.resolve_concept("instance"_view).resolve_concept("value"_view);
  const Abstract& static_value =
      packet.resolve_concept("static"_view).resolve_concept("value"_view);
  const Abstract& static_invoke =
      packet.resolve_concept("static"_view).resolve_concept("invoke"_view);
  const Abstract& self_invoke =
      packet.resolve_concept("instance"_view).resolve_concept("invoke"_view);
  ASSERT(field.is<Language::Field>());
  ASSERT(static_value.is<Language::Function>());
  ASSERT(static_invoke.is<Language::Function>());
  ASSERT(self_invoke.is<Language::Function>());
  EXPECT(&static_invoke != &self_invoke);
  EXPECT(packet.is_published(field));
  EXPECT(packet.is_published(static_value));
  EXPECT(packet.is_published(static_invoke));
  EXPECT(packet.is_published(self_invoke));

  const Abstract& hidden =
      packet.resolve_concept("instance"_view).resolve_concept("hidden"_view);
  ASSERT(hidden.is<Language::Field>());
  EXPECT_NOT(packet.is_published(hidden));

  EXPECT(
      packet.resolve_concept("Visible"_view).is<Language::Types::Structure>());
  EXPECT(packet.resolve_concept("Hidden"_view).is<Unknown>());
  EXPECT(packet.resolve_lexical_context("Hidden"_view)
             .is<Language::Types::Structure>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, field_access) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  private state value : Bool;\n"
      "  public read : func = [self] -> Bool { return self.value; }\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& selected = monograph->resolve_concept("Packet"_view);
  ASSERT(selected.is<Language::Types::Structure>());
  const auto& packet = static_cast<const Language::Types::Structure&>(selected);
  auto fields = packet.get_addressables();
  auto callables = packet.get_callables();
  ASSERT(fields != fields.end());
  const Abstract& field_identity = (*fields).get();
  ASSERT(callables != callables.end());
  ASSERT((*callables).get().is<Language::Function>());
  const auto& read = static_cast<const Language::Function&>((*callables).get());
  auto returned = find_return(read);
  ASSERT(returned);
  EXPECT_TEXT(
      returned->get_anchor().get_span().caculate_text(source),
      "return self.value;"_view);
  ASSERT_EQ(read.get_results().get_size(), Count(1));
  auto result_type = read.get_results().get_abstract(0);
  ASSERT(result_type);
  EXPECT(&*result_type == &monograph->resolve_concept("Bool"_view));
  const Abstract& receiver = read.resolve_concept("self"_view);
  auto receiver_result = receiver.select<Addressable>();
  ASSERT(receiver_result);
  EXPECT_TEXT(receiver_result->get_name(), "self"_view);
  EXPECT(&receiver_result->get_type() == &packet);
  auto layout_field = packet.get_layout().get_abstract(0);
  ASSERT(layout_field);
  EXPECT(&*layout_field == &field_identity);
  EXPECT(errors.is_empty());

  static constexpr Static::Vector<View::Bytes, 2> rejected = {{
    "// Missing receiver.\ndialect : Library;\npublic Packet : struct { public state value : Bool; public read : func = [] -> Bool { return value; } }"_view,
    "// Implicit receiver.\ndialect : Library;\npublic Packet : struct { public state value : Bool; public read : func = [self] -> Bool { return value; } }"_view,
  }};
  for (Count index = 0; index < rejected.get_size(); index++) {
    EXPECT(rejects_link(rejected[index]));
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, malformed_grammar) {
  static constexpr Static::Vector<View::Bytes, 9> sources = {{
    "// Structure test.\ndialect : Library; public Packet struct {}"_view,
    "// Structure test.\ndialect : Library; public Packet : wrong {}"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { value : Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public Value : Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Bool }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Core ::Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Core:: Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { using Core; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, missing_type) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Packet : struct { public missing : Missing; }"_view;
  EXPECT(rejects_link(source));
}

PERIMORTEM_UNIT_TEST(StructureTests, static_empty_types) {
  static constexpr Static::Vector<View::Bytes, 5> rejected = {{
    "// Empty Option element.\ndialect : Library; private Empty : struct {} private invalid : Option[Empty];"_view,
    "// Zero Fixed extent.\ndialect : Library; private invalid : Fixed[Bool, 0];"_view,
    "// Empty Composite Addressable.\ndialect : Library; private Empty : struct {} private invalid : Empty;"_view,
    "// Empty Self receiver.\ndialect : Library; public Empty : struct { public invalid : func = [self] -> [] {} }"_view,
    "// Empty named parameter.\ndialect : Library; private Empty : struct {} private invalid : func = [.value : Empty] -> [] {}"_view,
  }};
  EXPECT(rejects_link(rejected[0]));
  EXPECT(rejects_link(rejected[1]));
  EXPECT(rejects_link(rejected[2]));
  EXPECT(rejects_link(rejected[3]));
  EXPECT(rejects_link(rejected[4]));

  static constexpr View::Bytes source =
      "// Empty Types retain only Static bindings.\n"
      "dialect : Library;\n"
      "public Empty : struct { public create : func = [] -> [] {} }\n"
      "private first_empty_result : func = [] -> [] {}\n"
      "private empty_result : func = [] -> [] {}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& empty_identity = monograph->resolve_concept("Empty"_view);
  ASSERT(empty_identity.is<Language::Types::Structure>());
  const auto& empty =
      static_cast<const Language::Types::Structure&>(empty_identity);
  EXPECT(empty.get_layout().is_empty());
  auto empty_callables = empty.get_callables();
  ASSERT(empty_callables != empty_callables.end());
  EXPECT_TEXT((*empty_callables).get().get_name(), "create"_view);
  ++empty_callables;
  EXPECT(empty_callables == empty.get_callables().end());

  auto callables = monograph->get_source().get_callables();
  ASSERT(callables != callables.end());
  ASSERT((*callables).get().is<Language::Function>());
  const auto& first_empty_result =
      static_cast<const Language::Function&>((*callables).get());
  ++callables;
  ASSERT(callables != monograph->get_source().get_callables().end());
  ASSERT((*callables).get().is<Language::Function>());
  const auto& empty_result =
      static_cast<const Language::Function&>((*callables).get());
  ++callables;
  EXPECT(callables == monograph->get_source().get_callables().end());
  EXPECT(first_empty_result.get_results().is_empty());
  EXPECT(empty_result.get_results().is_empty());
  EXPECT(first_empty_result.get_results().fits(empty_result.get_results()));
  EXPECT(empty_result.get_results().fits(first_empty_result.get_results()));

  const auto& scalar = static_cast<const Tetrodotoxin::Source::Type&>(
      monograph->resolve_concept("U8"_view));
  ASSERT_EQ(scalar.get_layout().get_size(), Count(1));
  auto scalar_layout_type = scalar.get_layout().get_abstract(0);
  ASSERT(scalar_layout_type);
  EXPECT(&*scalar_layout_type == &scalar);
  EXPECT_NOT(first_empty_result.get_results().fits(scalar.get_layout()));
  EXPECT_NOT(scalar.get_layout().fits(first_empty_result.get_results()));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, public_exposure) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "private Hidden : struct { private state value : Bool; }\n"
    "public Packet : struct { public state hidden : Hidden; }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "private Hidden : struct { private state value : Bool; }\n"
    "public Packet : struct { public reveal : func = [.value : Hidden] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "private Hidden : struct { private state value : Bool; }\n"
    "public Packet : struct {\n"
    "  private state hidden : Hidden;\n"
    "  public reveal : func = [self] -> Hidden { return self.hidden; }\n"
    "}"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public reveal : func = [.value : Hidden] -> [] {}\n"
    "private Hidden : struct { private state value : Bool; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_finalize(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, private_surface) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "private Hidden : struct { private state value : Bool; }\n"
      "public Packet : struct {\n"
      "  private state hidden : Hidden;\n"
      "  private reveal : func = [.value : Hidden] -> Hidden { return value; "
      "}\n"
      "}\n"
      "private root : func = [.value : Hidden] -> Hidden { return value; }"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  const auto& source_type = monograph->get_source();
  auto types = source_type.get_types();
  auto source_callables = source_type.get_callables();
  ASSERT(types != types.end());
  ASSERT((*types).get().is<Language::Types::Structure>());
  const auto& hidden =
      static_cast<const Language::Types::Structure&>((*types).get());
  const Abstract& packet_identity = monograph->resolve_concept("Packet"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  ASSERT(source_callables != source_callables.end());
  ASSERT((*source_callables).get().is<Language::Function>());
  const auto& root =
      static_cast<const Language::Function&>((*source_callables).get());
  EXPECT(&monograph->resolve_concept("Hidden"_view) == &Unknown::get_unknown());
  EXPECT(&monograph->resolve_concept("root"_view) == &Unknown::get_unknown());
  EXPECT(&root.resolve_concept("Hidden"_view) == &hidden);
  EXPECT(&packet.resolve_concept("hidden"_view) == &Unknown::get_unknown());
  EXPECT(&packet.resolve_concept("reveal"_view) == &Unknown::get_unknown());
  auto fields = packet.get_addressables();
  auto callables = packet.get_callables();
  ASSERT(fields != fields.end());
  ASSERT(callables != callables.end());
  ASSERT((*callables).get().is<Language::Function>());
  const auto& reveal =
      static_cast<const Language::Function&>((*callables).get());
  EXPECT(&reveal.resolve_concept("hidden"_view) == &Unknown::get_unknown());
  EXPECT(&reveal.resolve_concept("Hidden"_view) == &hidden);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, initializer_fitting) {
  static constexpr View::Bytes source =
      "// Structure initializer test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  private exact : Bool = false;\n"
      "  private copy : Bool = exact;\n"
      "  private narrow : U8 = 255;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto& dialect = get_library_dialect(*workspace_toolchain);
  Allocator::Arena lexical;
  auto owner = parse_authored(lexical, dialect, workspace, errors, source);
  ASSERT(owner);
  auto& monograph = *owner;
  const Abstract& selected = monograph.resolve_concept("Packet"_view);
  ASSERT(selected.is<Language::Types::Structure>());
  const auto& packet = static_cast<const Language::Types::Structure&>(selected);
  auto authored_fields = packet.get_addressables();
  ASSERT(authored_fields != authored_fields.end());
  const auto& exact =
      static_cast<const Language::Field&>((*authored_fields).get());
  ++authored_fields;
  ASSERT(authored_fields != authored_fields.end());
  const auto& copy =
      static_cast<const Language::Field&>((*authored_fields).get());
  ++authored_fields;
  ASSERT(authored_fields != authored_fields.end());
  const auto& narrow =
      static_cast<const Language::Field&>((*authored_fields).get());
  auto exact_initializer = exact.get_initializer();
  auto copy_initializer = copy.get_initializer();
  auto narrow_initializer = narrow.get_initializer();
  ASSERT(exact_initializer);
  ASSERT(copy_initializer);
  ASSERT(narrow_initializer);

  Allocator::Arena completion;
  Tokenizer tokenizer(completion, source, "structure.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph.link(cursor));
  ASSERT(monograph.finalize(cursor));

  auto retained_exact_initializer = exact.get_initializer();
  auto retained_copy_initializer = copy.get_initializer();
  auto retained_narrow_initializer = narrow.get_initializer();
  ASSERT(retained_exact_initializer);
  ASSERT(retained_copy_initializer);
  ASSERT(retained_narrow_initializer);
  EXPECT(&*retained_exact_initializer == &*exact_initializer);
  EXPECT(&*retained_copy_initializer == &*copy_initializer);
  EXPECT(&*retained_narrow_initializer == &*narrow_initializer);
  ASSERT(copy_initializer->is_identity<Language::Expressions::Identifier>());
  const auto& identifier =
      static_cast<const Language::Expressions::Identifier&>(*copy_initializer);
  EXPECT(&identifier.get_result() == &exact);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, inferred_fields) {
  static constexpr View::Bytes source =
      "// Field inference test.\n"
      "dialect : Library;\n"
      "private root : Bool = false;\n"
      "private root_copy := root;\n"
      "public Packet : struct {\n"
      "  private scalar := 7;\n"
      "  private scalar_copy := scalar;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto& dialect = get_library_dialect(*workspace_toolchain);
  Allocator::Arena lexical;
  auto owner = parse_authored(lexical, dialect, workspace, errors, source);
  ASSERT(owner);
  auto& monograph = *owner;
  const auto& source_type = monograph.get_source();
  auto types = source_type.get_types();
  auto type = types.begin();
  ASSERT(type != types.end());
  ASSERT((*type).get().is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>((*type).get());
  ++type;
  EXPECT(type == types.end());

  Allocator::Arena completion;
  Tokenizer tokenizer(completion, source, "structure.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph.link(cursor));
  auto source_fields = source_type.get_addressables();
  auto packet_fields = packet.get_addressables();
  auto source_field = source_fields.begin();
  ASSERT(source_field != source_fields.end());
  const auto& root = static_cast<const Language::Field&>((*source_field).get());
  ++source_field;
  ASSERT(source_field != source_fields.end());
  const auto& root_copy =
      static_cast<const Language::Field&>((*source_field).get());
  ++source_field;
  EXPECT(source_field == source_fields.end());
  auto packet_field = packet_fields.begin();
  ASSERT(packet_field != packet_fields.end());
  const auto& scalar =
      static_cast<const Language::Field&>((*packet_field).get());
  ++packet_field;
  ASSERT(packet_field != packet_fields.end());
  const auto& scalar_copy =
      static_cast<const Language::Field&>((*packet_field).get());
  ++packet_field;
  EXPECT(packet_field == packet_fields.end());
  EXPECT(&root_copy.get_type() == &root.get_type());
  EXPECT(&root.get_type() == &monograph.resolve_concept("Bool"_view));
  EXPECT(&scalar_copy.get_type() == &scalar.get_type());
  EXPECT(&scalar.get_type() == &monograph.resolve_concept("U64"_view));
  auto root_copy_initializer = root_copy.get_initializer();
  ASSERT(root_copy_initializer);
  ASSERT(
      root_copy_initializer->is_identity<Language::Expressions::Identifier>());
  const auto& root_identifier =
      static_cast<const Language::Expressions::Identifier&>(
          *root_copy_initializer);
  EXPECT(&root_identifier.get_result() == &root);

  const Language::Field* source_identity = &root_copy;
  const Language::Field* nested_identity = &scalar_copy;
  ASSERT(monograph.link(cursor));
  ASSERT(monograph.link(cursor));
  auto retained_source_fields = source_type.get_addressables();
  auto retained_source_field = retained_source_fields.begin();
  ASSERT(retained_source_field != retained_source_fields.end());
  ++retained_source_field;
  ASSERT(retained_source_field != retained_source_fields.end());
  EXPECT(&(*retained_source_field).get() == source_identity);
  ++retained_source_field;
  EXPECT(retained_source_field == retained_source_fields.end());
  auto retained_packet_fields = packet.get_addressables();
  auto retained_packet_field = retained_packet_fields.begin();
  ASSERT(retained_packet_field != retained_packet_fields.end());
  ++retained_packet_field;
  ASSERT(retained_packet_field != retained_packet_fields.end());
  EXPECT(&(*retained_packet_field).get() == nested_identity);
  ++retained_packet_field;
  EXPECT(retained_packet_field == retained_packet_fields.end());
  ASSERT(monograph.finalize(cursor));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, inference_rollback) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Field inference test.\ndialect : Library; private value := missing;"_view,
    "// Field inference test.\ndialect : Library; private value := true + false;"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Tetrodotoxin::Library::Dialect workspace_toolchain_library;
    auto workspace_toolchain =
        Validation::create_library_toolchain(workspace_toolchain_library);
    Workspace workspace(*workspace_toolchain);
    Errors errors;
    auto& dialect = get_library_dialect(*workspace_toolchain);
    Allocator::Arena lexical;
    auto owner =
        parse_authored(lexical, dialect, workspace, errors, sources[i]);
    ASSERT(owner);
    auto& monograph = *owner;
    const auto& source_type = monograph.get_source();
    Allocator::Arena completion;
    Tokenizer tokenizer(completion, sources[i], "structure.ttx"_view);
    Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
    EXPECT_NOT(monograph.link(cursor));
    auto fields = source_type.get_addressables();
    auto field = fields.begin();
    ASSERT(field != fields.end());
    const auto* identity = &static_cast<const Language::Field&>((*field).get());
    ++field;
    EXPECT(field == fields.end());
    EXPECT(&identity->resolve() == &Unknown::get_unknown());
    EXPECT_NOT(monograph.link(cursor));
    auto retained_fields = source_type.get_addressables();
    auto retained_field = retained_fields.begin();
    ASSERT(retained_field != retained_fields.end());
    EXPECT(&(*retained_field).get() == identity);
    ++retained_field;
    EXPECT(retained_field == retained_fields.end());
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, public_inference) {
  static constexpr View::Bytes source =
      "// Field inference test.\n"
      "dialect : Library;\n"
      "private Hidden : struct { private state value : Bool; }\n"
      "private seed : Hidden;\n"
      "public revealed := seed;"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto& dialect = get_library_dialect(*workspace_toolchain);
  Allocator::Arena lexical;
  auto owner = parse_authored(lexical, dialect, workspace, errors, source);
  ASSERT(owner);
  auto& monograph = *owner;
  Allocator::Arena completion;
  Tokenizer tokenizer(completion, source, "structure.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  ASSERT(monograph.link(cursor));
  const auto& source_type = monograph.get_source();
  auto fields = source_type.get_addressables();
  auto field = fields.begin();
  ASSERT(field != fields.end());
  ++field;
  ASSERT(field != fields.end());
  ++field;
  EXPECT(field == fields.end());
  EXPECT_NOT(monograph.finalize(cursor));
  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(has_diagnostic(
      errors, "Externally readable Field publishes an unreachable Type."_view));
}

PERIMORTEM_UNIT_TEST(StructureTests, initializer_mismatch) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Structure initializer test.\ndialect : Library; public Packet : struct { private value : U8 = false; }"_view,
    "// Structure initializer test.\ndialect : Library; public Packet : struct { private value : U8 = 256; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Tetrodotoxin::Library::Dialect workspace_toolchain_library;
    auto workspace_toolchain =
        Validation::create_library_toolchain(workspace_toolchain_library);
    Workspace workspace(*workspace_toolchain);
    Errors errors;
    auto& dialect = get_library_dialect(*workspace_toolchain);
    Allocator::Arena lexical;
    auto owner =
        parse_authored(lexical, dialect, workspace, errors, sources[i]);
    ASSERT(owner);
    auto& monograph = *owner;
    const auto& source_type = monograph.get_source();
    auto types = source_type.get_types();
    auto type = types.begin();
    ASSERT(type != types.end());
    ASSERT((*type).get().is<Language::Types::Structure>());
    const auto& packet =
        static_cast<const Language::Types::Structure&>((*type).get());
    ++type;
    EXPECT(type == types.end());
    auto authored_fields = packet.get_addressables();
    auto authored_field_selection = authored_fields.begin();
    ASSERT(authored_field_selection != authored_fields.end());
    const auto& authored_field =
        static_cast<const Language::Field&>((*authored_field_selection).get());
    ++authored_field_selection;
    EXPECT(authored_field_selection == authored_fields.end());
    auto authored_initializer = authored_field.get_initializer();
    ASSERT(authored_initializer);

    Allocator::Arena completion;
    Tokenizer tokenizer(completion, sources[i], "structure.ttx"_view);
    Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
    EXPECT_NOT(monograph.link(cursor));
    auto fields = packet.get_addressables();
    auto field_selection = fields.begin();
    ASSERT(field_selection != fields.end());
    const auto& retained_field =
        static_cast<const Language::Field&>((*field_selection).get());
    ++field_selection;
    EXPECT(field_selection == fields.end());
    auto retained_initializer = retained_field.get_initializer();
    ASSERT(retained_initializer);
    EXPECT(&*retained_initializer == &*authored_initializer);
    ASSERT_EQ(errors.get_size(), Count(1));
    EXPECT(has_diagnostic(
        errors, i == 0 ? "private value : U8 = false;"_view
                       : "private value : U8 = 256;"_view));
    EXPECT(has_diagnostic(
        errors,
        "Initializer for Field 'value' does not fit declared Type "
        "'U8'."_view));
    EXPECT(has_diagnostic(errors, "Target accepts: [U8]"_view));
    EXPECT(has_diagnostic(
        errors,
        "Change the initializer or declare the exact Type it produces."_view));
  }
}
