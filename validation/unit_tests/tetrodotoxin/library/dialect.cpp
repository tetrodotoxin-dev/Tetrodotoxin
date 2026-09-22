// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/dialect.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/access/slice.hpp"
#include "tetrodotoxin/library/language/access/swizzle.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/flow/branch.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/flow/loop_control.hpp"
#include "tetrodotoxin/library/language/flow/match.hpp"
#include "tetrodotoxin/library/language/flow/range_loop.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/foreign.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/operations/add_assignment.hpp"
#include "tetrodotoxin/library/language/operations/assignment.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "tetrodotoxin/terminal/llvm/compiler.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
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

class EmptyRegistry : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "EmptyRegistry"_view;
  }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes) const -> const Abstract& override {
    return Unknown::get_unknown();
  }
};

class EmbeddedResource final : public Tetrodotoxin::Language::Resource {
 public:
  constexpr EmbeddedResource(View::Bytes value) : value(value) {}

  constexpr auto get_value() const -> View::Bytes override { return value; }

 private:
  View::Bytes value;
};

class ResourceRegistry final : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "ResourceRegistry"_view;
  }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes route) const -> const Abstract& override {
    if (route == "$[resource/hello.txt]"_view) {
      return table;
    }
    if (route == "$[resource/greeting.txt]"_view) {
      return greeting;
    }
    return Unknown::get_unknown();
  }

 private:
  EmbeddedResource table{"0123456789ABCDEF"_view};
  EmbeddedResource greeting{"Hello World"_view};
};

class FutureType : public Language::Model::Type {
 public:
  constexpr FutureType(View::Bytes name = "Future"_view) : name(name) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes) const -> const Abstract& override {
    return Unknown::get_unknown();
  }
  auto create_default(Perimortem::Memory::Allocator::Arena&) const
      -> Option<Language::Model::Pack&> override {
    return {};
  }

 private:
  View::Bytes name;
};

class QualifiedContext : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "Types"_view;
  }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes route) const -> const Abstract& override {
    if (route == qualified.get_name()) {
      return qualified;
    }

    return Unknown::get_unknown();
  }

 private:
  FutureType qualified{"Qualified"_view};
};

class NonTypeFact : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "Fact"_view;
  }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes) const -> const Abstract& override {
    return Unknown::get_unknown();
  }
};

class AliasContext : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "AliasContext"_view;
  }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes route) const -> const Abstract& override {
    if (route == types.get_name()) {
      return types;
    }
    if (route == outer.get_name()) {
      return outer;
    }
    if (route == fact.get_name()) {
      return fact;
    }

    return Unknown::get_unknown();
  }

 private:
  QualifiedContext types;
  FutureType outer{"Outer"_view};
  NonTypeFact fact;
};

static auto import_library(
    Workspace& workspace,
    Errors& errors,
    View::Bytes semantic_name,
    View::Bytes source) -> Option<Language::Monograph&> {
  auto imported =
      workspace.interpret_source(errors, semantic_name, semantic_name, source);
  if (!imported || !imported->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*imported);
}

static auto rejects_library_source(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "RejectedLibrary"_view, "rejected-library.ttx"_view, source);
  return !interpreted && !errors.is_empty();
}

static auto interpret_library_source(
    Allocator::Arena& domain,
    Dialect& dialect,
    Cursor& cursor,
    Abstract& context) -> Option<Language::Monograph&> {
  (void)domain;
  Anchor source_anchor = Anchor::create(Span());
  Count error_count = cursor.get_error_count();
  auto interpretation = dialect.interpret(
      cursor, Tetrodotoxin::Source::Documentation::get_empty(), source_anchor, context);
  BAIL_IF(
      !interpretation || cursor.get_error_count() != error_count ||
      !interpretation->is<Language::Monograph>());
  return static_cast<Language::Monograph&>(*interpretation);
}

static auto select_library_monograph(Option<Language::Monograph&>& owner)
    -> Option<Language::Monograph&> {
  return owner;
}

static Harness DialectTests = {
  .name = "Tetrodotoxin::Library::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(DialectTests, root_vocabulary) {
  Allocator::Arena arena;
  EmptyRegistry context;
  Dialect dialect;
  Dialect same_type;
  const Tetrodotoxin::Source::Documentation& documentation = Tetrodotoxin::Source::Documentation::get_empty();
  Anchor source_anchor = Anchor::create(Span());

  Errors errors;
  Tokenizer tokenizer(arena, {}, "embedded-library.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto& first = Language::Monograph::create_authored(
      cursor.get_arena(), documentation, source_anchor, dialect, context);
  auto& second = Language::Monograph::create_authored(
      cursor.get_arena(), documentation, source_anchor, dialect, context);

  EXPECT_NOT(dialect.encode(first));

  EXPECT(&first.get_documentation() == &documentation);
  EXPECT(&first.get_source().get_documentation() == &documentation);
  EXPECT(&first.get_language() == &dialect);
  ASSERT(first.get_layer(dialect));
  EXPECT(&*first.get_layer(dialect) == &first);
  EXPECT_NOT(first.get_layer(same_type));
  EXPECT(&first != &second);
  EXPECT(&first.get_source() != &second.get_source());

  const auto& first_element = static_cast<const Language::Model::Type&>(
      first.resolve_concept("U8"_view));
  const auto& second_element = static_cast<const Language::Model::Type&>(
      second.resolve_concept("U8"_view));
  const auto& first_formula =
      static_cast<const Language::Generic&>(first.resolve_concept("View"_view));
  const auto& second_formula = static_cast<const Language::Generic&>(
      second.resolve_concept("View"_view));
  Static::Vector<Language::Generic::Argument, 1> first_arguments = {{
    Language::Generic::Argument(first_element),
  }};
  Static::Vector<Language::Generic::Argument, 1> second_arguments = {{
    Language::Generic::Argument(second_element),
  }};
  auto first_result = first_formula.materialize(first_arguments.get_view());
  auto repeated_result = first_formula.materialize(first_arguments.get_view());
  auto second_result = second_formula.materialize(second_arguments.get_view());
  Option<const Language::Model::Type&> first_view;
  Option<const Language::Model::Type&> repeated_view;
  Option<const Language::Model::Type&> second_view;
  first_result.visit(
      [&](const Language::Model::Type& selected) { first_view = selected; },
      [](const Language::Generic::Failure&) {});
  repeated_result.visit(
      [&](const Language::Model::Type& selected) { repeated_view = selected; },
      [](const Language::Generic::Failure&) {});
  second_result.visit(
      [&](const Language::Model::Type& selected) { second_view = selected; },
      [](const Language::Generic::Failure&) {});
  ASSERT(first_view && repeated_view && second_view);
  EXPECT(&*first_view == &*repeated_view);
  EXPECT(&*first_view != &*second_view);
}

PERIMORTEM_UNIT_TEST(DialectTests, progressive_source) {
  static constexpr View::Bytes source =
      "// Progressive Library source.\n"
      "dialect : Library;\n"
      "public ready : U64;\n"
      "public pending : Missing::;\n"
      "public later : U64;\n"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;

  auto completed = workspace.interpret_source(
      errors, "Progressive"_view, "progressive.ttx"_view, source);
  EXPECT_NOT(completed);
  EXPECT_NOT(errors.is_empty());

  auto monograph = workspace.resolve_concept("Progressive"_view)
                       .select<Language::Monograph>();
  ASSERT(monograph);
  const auto& root = monograph->get_source();
  const Abstract& ready = root.resolve_local(
      "ready"_view, Tetrodotoxin::Language::Visibility::Private);
  const Abstract& pending = root.resolve_local(
      "pending"_view, Tetrodotoxin::Language::Visibility::Private);
  const Abstract& later = root.resolve_local(
      "later"_view, Tetrodotoxin::Language::Visibility::Private);
  EXPECT(ready.is<Language::Field>());
  ASSERT(pending.is<Language::Field>());
  EXPECT(later.is<Language::Field>());
  const auto& pending_field = static_cast<const Language::Field&>(pending);
  EXPECT_NOT(pending_field.get_type_reference());
  EXPECT(pending_field.resolve().is<Unknown>());
  EXPECT(workspace.get_associations("progressive.ttx"_view));
}

static auto find_field(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Field&> {
  auto fields = composite.get_addressables();
  for (auto field = fields.begin(); field != fields.end(); ++field) {
    const Abstract& candidate = (*field).get();
    if (candidate.get_name() == name && candidate.is<Language::Field>()) {
      return static_cast<const Language::Field&>(candidate);
    }
  }

  return {};
}

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  auto functions = composite.get_callables();
  for (auto function = functions.begin(); function != functions.end();
       ++function) {
    const Abstract& candidate = (*function).get();
    if (candidate.get_name() == name && candidate.is<Language::Function>()) {
      return static_cast<const Language::Function&>(candidate);
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(DialectTests, missing_type_route) {
  static constexpr View::Bytes source =
      "public Packet : struct { public missing : Missing; }\n"
      "public later : func = [] -> [] {}"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "phase-cascade.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto interpreted_owner =
      interpret_library_source(arena, dialect, cursor, registry);
  auto interpreted = select_library_monograph(interpreted_owner);
  ASSERT(interpreted);
  auto& monograph = *interpreted;
  ASSERT_NOT(monograph.link(cursor));
  ASSERT_EQ(errors.get_size(), Count(1));
  Allocator::Arena rendered;
  EXPECT(
      Algorithm::search(errors.render_message(rendered, 0), "Missing"_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, qualified_type_associations) {
  static constexpr View::Bytes source =
      "public Outer : struct {\n"
      "  public Inner : struct { public state member : U64; }\n"
      "}\n"
      "private value : Outer::Inner;"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "qualified-type-associations.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto interpreted_owner =
      interpret_library_source(arena, dialect, cursor, registry);
  auto interpreted = select_library_monograph(interpreted_owner);
  ASSERT(interpreted);
  auto& monograph = *interpreted;
  ASSERT(monograph.link(cursor));

  const Abstract& outer = monograph.resolve_concept("Outer"_view);
  const Abstract& inner = outer.resolve_concept("Inner"_view);
  Count route = Algorithm::search(source, "Outer::Inner"_view);
  ASSERT(route != Count(-1));
  auto selected_outer = associations.find_at(route);
  auto selected_inner = associations.find_at(route + 7);
  ASSERT(selected_outer && selected_inner);
  EXPECT(&*selected_outer == &outer);
  EXPECT(&*selected_inner == &inner);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, field_diagnostics) {
  static constexpr Static::Vector<View::Bytes, 6> sources = {{
    "public broken : Missing;\npublic later : func = [] -> [] {}"_view,
    "public broken : Bool = absent;\npublic later : func = [] -> [] {}"_view,
    "public const broken : Bool = 1;\n"
    "public later : func = [] -> [] {}"_view,
    "public const broken : U8 = 256;\n"
    "public later : func = [] -> [] {}"_view,
    "public dynamic : Bool = false;\n"
    "public const broken := dynamic;"_view,
    "public Packet : struct {\n"
    "  public dynamic : Bool = false;\n"
    "  public const broken := dynamic;\n"
    "}"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Allocator::Arena arena;
    EmptyRegistry registry;
    Dialect dialect;
    Errors errors;
    Tokenizer tokenizer(arena, sources[i], "source-field-failure.ttx"_view);
    Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
    auto interpreted_owner =
        interpret_library_source(arena, dialect, cursor, registry);
    auto interpreted = select_library_monograph(interpreted_owner);
    ASSERT(interpreted);
    auto& monograph = *interpreted;
    ASSERT_NOT(monograph.link(cursor));
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, source_rejections) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Top level Self registration.\ndialect : Library;\npublic invalid : func = [self] -> [] {}"_view,
    "// Source state rejection.\ndialect : Library;\npublic state invalid : Bool;"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_library_source(sources[index]));
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, source_aliases) {
  static constexpr View::Bytes source =
      "// Hidden documentation.\n"
      "private Hidden : struct {}\n"
      "// Local documentation.\n"
      "public PublicAlias : alias = Hidden;\n"
      "private PrivateAlias : alias = PublicAlias;"_view;
  Allocator::Arena arena;
  EmptyRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "source-alias.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto interpreted_owner =
      interpret_library_source(arena, dialect, cursor, registry);
  auto interpreted = select_library_monograph(interpreted_owner);
  ASSERT(interpreted);
  auto& monograph = *interpreted;
  const auto& source_type = monograph.get_source();
  auto types = source_type.get_types();
  auto type = types.begin();
  ASSERT(type != types.end());
  const Abstract& hidden = (*type).get();
  ++type;
  ASSERT(type != types.end());
  const Abstract& public_identity = (*type).get();
  ++type;
  ASSERT(type != types.end());
  const Abstract& private_identity = (*type).get();
  ++type;
  EXPECT(type == types.end());
  ASSERT(hidden.is<Language::Model::Type>());
  ASSERT(public_identity.is<Alias>());
  ASSERT(private_identity.is<Alias>());
  const auto& public_alias = static_cast<const Alias&>(public_identity);
  const auto& private_alias = static_cast<const Alias&>(private_identity);

  ASSERT(monograph.link(cursor));
  ASSERT(monograph.finalize(cursor));
  EXPECT(&public_alias.resolve() == &hidden);
  EXPECT(&private_alias.resolve() == &hidden);
  const Tetrodotoxin::Source::Documentation& public_documentation = public_alias.get_documentation();
  ASSERT_EQ(public_documentation.line_count(), Count(2));
  EXPECT_TEXT(public_documentation.get_line(0), "Local documentation."_view);
  EXPECT_TEXT(public_documentation.get_line(1), "Hidden documentation."_view);
  EXPECT(&monograph.resolve_concept("PublicAlias"_view) == &public_identity);
  EXPECT(
      &monograph.resolve_concept("PrivateAlias"_view) ==
      &Unknown::get_unknown());
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(DialectTests, alias_rejection) {
  static constexpr Static::Vector<View::Bytes, 4> rejected = {{
    "// Rejected documentation.\npublic Broken : alias Bool;"_view,
    "public Bool : alias = U8;"_view,
    "public Broken : alias = Bool"_view,
    "public First : alias = Bool; public First : alias = U8;"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    Allocator::Arena arena;
    AliasContext registry;
    Dialect dialect;
    Errors errors;
    Tokenizer tokenizer(arena, rejected[i], "rejected-source-alias.ttx"_view);
    Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
    auto interpreted_owner =
        interpret_library_source(arena, dialect, cursor, registry);
    auto interpreted = select_library_monograph(interpreted_owner);
    EXPECT_NOT(interpreted);
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, delayed_aliases) {
  static constexpr Static::Vector<View::Bytes, 3> rejected = {{
    "public Broken : alias = Missing;"_view,
    "public Broken : alias = Fact;"_view,
    "public Self : alias = Self;"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    Allocator::Arena arena;
    AliasContext registry;
    Dialect dialect;
    Errors errors;
    Tokenizer tokenizer(arena, rejected[i], "delayed-source-alias.ttx"_view);
    Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
    auto interpreted_owner =
        interpret_library_source(arena, dialect, cursor, registry);
    auto interpreted = select_library_monograph(interpreted_owner);
    ASSERT(interpreted);
    auto& monograph = *interpreted;
    EXPECT(errors.is_empty());
    EXPECT_NOT(monograph.link(cursor));
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, fixture_rejections) {
  struct Rejection {
    View::Bytes path;
    View::Bytes message;
    View::Bytes source_line;
  };
  static constexpr Static::Vector<Rejection, 4> rejections = {{
    Rejection{
      "validation/data/ttx/library/dialect_led_callable.ttx"_view,
      "Definitions require one authored visibility before their name."_view,
      "Library legacy[] -> [] : return;"_view,
    },
    {
      "validation/data/ttx/library/duplicate_name.ttx"_view,
      "Library Addressable name is already occupied in this Composite."_view,
      "public duplicate : U64 = 2;"_view,
    },
    {
      "validation/data/ttx/library/bare_new.ttx"_view,
      "Library `new` requires `[` before its Object Type."_view,
      "private inferred := new;"_view,
    },
    {
      "validation/data/ttx/library/ordinary_bodyless.ttx"_view,
      "Library Blocks require `{` for several Statements or `:` for one "
      "Statement."_view,
      "public missing_body : func = [] -> U64;"_view,
    },
  }};

  for (Count i = 0; i < rejections.get_size(); i++) {
    const Rejection& rejection = rejections[i];
    auto source = File::read(rejection.path);
    ASSERT(source);

    Tetrodotoxin::Library::Dialect workspace_toolchain_library;

    auto workspace_toolchain =
        Validation::create_library_toolchain(workspace_toolchain_library);

    Workspace workspace(*workspace_toolchain);
    Errors errors;

    auto interpreted = workspace.interpret_source(
        errors, "Rejected"_view, rejection.path, *source);
    Bool completed = interpreted && errors.is_empty();

    EXPECT_NOT(completed);
    EXPECT_EQ(errors.get_size(), Count(1));

    Allocator::Arena rendered_domain;
    View::Bytes rendered = errors.render_message(rendered_domain, 0);
    EXPECT(Algorithm::search(rendered, rejection.message) != Count(-1));
    EXPECT(Algorithm::search(rendered, rejection.source_line) != Count(-1));
  }
}

PERIMORTEM_UNIT_TEST(DialectTests, foreign_workspace) {
  static constexpr View::Bytes path =
      "validation/data/ttx/products/foreign/foreign.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Tetrodotoxin::Library::Dialect workspace_toolchain_library;

  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);

  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "ForeignAcceptance"_view, path, *source);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  const Language::Foreign& foreign = monograph.get_source().get_foreign();
  const Abstract& readonly = foreign.resolve_concept("static"_view)
                                 .resolve_concept("library_foreign_bias"_view);
  const Abstract& state = foreign.resolve_concept("static"_view)
                              .resolve_concept("library_foreign_state"_view);
  const Abstract& function = foreign.resolve_concept("static"_view)
                                 .resolve_concept("library_foreign_add"_view);
  EXPECT(monograph.resolve_concept("library_foreign_state"_view).is<Unknown>());
  EXPECT(readonly.is<Addressable>());
  EXPECT(state.is<Addressable>());
  EXPECT(function.is<Callable>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, private_parameter) {
  static constexpr View::Bytes path =
      "validation/data/ttx/library/public_parameter_private_type.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Tetrodotoxin::Library::Dialect workspace_toolchain_library;

  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);

  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "PrivateParameter"_view, path, *source);
  EXPECT_NOT(interpreted);
  EXPECT(retains_library_source(workspace, "PrivateParameter"_view));
  ASSERT_EQ(errors.get_size(), Count(1));
  Allocator::Arena rendered;
  View::Bytes message = errors.render_message(rendered, 0);
  EXPECT(Algorithm::search(message, "Hidden"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          message,
          "Externally readable Function publishes an unreachable Type "
          "route."_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          message,
          "Keep the Function private or publish its authored Type route."_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, opaque_attributes) {
  static constexpr View::Bytes source =
      "// Function Attribute retention test.\n"
      "dialect : Library;\n"
      "@extension(\"first\")\n"
      "@extension(\"second\")\n"
      "@abi(\"Rust\")\n"
      "@abi(1)\n"
      "@symbol(\"\")\n"
      "@symbol(\"next\")\n"
      "public first : func = [] -> Bool { return true; }\n"
      "@symbol(\"without_abi\")\n"
      "private second : func = [] -> Bool { return false; }\n"
      "public Host : struct {\n"
      "  public state value : Bool;\n"
      "  @abi(\"C\")\n"
      "  @symbol(\"member\")\n"
      "  public member : func = [self] -> Bool { return true; }\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = import_library(workspace, errors, "Attributes"_view, source);
  ASSERT(monograph);

  const Abstract& first_identity =
      monograph->resolve_concept("static"_view).resolve_concept("first"_view);
  ASSERT(first_identity.is<Language::Function>());
  const auto& first = static_cast<const Language::Function&>(first_identity);
  auto first_attributes = first.get_definition().get_attributes();
  ASSERT_EQ(first_attributes.get_size(), Count(6));
  EXPECT_TEXT(first_attributes.get_data()[0].get_key(), "extension"_view);
  EXPECT_TEXT(first_attributes.get_data()[1].get_key(), "extension"_view);
  EXPECT_TEXT(first_attributes.get_data()[2].get_key(), "abi"_view);
  EXPECT_TEXT(first_attributes.get_data()[3].get_key(), "abi"_view);
  EXPECT_TEXT(first_attributes.get_data()[4].get_key(), "symbol"_view);
  EXPECT_TEXT(first_attributes.get_data()[5].get_key(), "symbol"_view);
  const View::Bytes* first_extension =
      first_attributes.get_data()[0].get_value().find<View::Bytes>();
  const View::Bytes* second_extension =
      first_attributes.get_data()[1].get_value().find<View::Bytes>();
  const View::Bytes* empty_symbol =
      first_attributes.get_data()[4].get_value().find<View::Bytes>();
  ASSERT(first_extension);
  ASSERT(second_extension);
  ASSERT(empty_symbol);
  EXPECT_TEXT(*first_extension, "first"_view);
  EXPECT_TEXT(*second_extension, "second"_view);
  EXPECT(empty_symbol->is_empty());

  const Abstract& host_identity = monograph->resolve_concept("Host"_view);
  ASSERT(host_identity.is<Language::Types::Structure>());
  const auto& host =
      static_cast<const Language::Types::Structure&>(host_identity);
  const Abstract& member_identity =
      host.resolve_concept("instance"_view).resolve_concept("member"_view);
  ASSERT(member_identity.is<Language::Function>());
  const auto& member = static_cast<const Language::Function&>(member_identity);
  ASSERT_EQ(member.get_definition().get_attributes().get_size(), Count(2));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, source_acceptance) {
  static constexpr View::Bytes path =
      "validation/data/ttx/library/source_acceptance.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Tetrodotoxin::Library::Dialect workspace_toolchain_library;

  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);

  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "SourceAcceptance"_view, path, *source);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  EXPECT_TEXT(
      monograph.get_documentation().get_line(1),
      "Library source acceptance."_view);

  EXPECT(&workspace.resolve_concept("SourceAcceptance"_view) == &monograph);

  const auto& source_type = monograph.get_source();
  EXPECT_TEXT(source_type.get_name(), "<source>"_view);
  EXPECT(&source_type.get_definition().get_host() == &monograph);
  EXPECT_NOT(source_type.get_definition().get_authored().is_authored());
  EXPECT(source_type.get_definition().is_published());
  const Anchor source_anchor = source_type.get_anchor();
  EXPECT_TEXT(source_anchor.get_token().caculate_text(*source), "dialect"_view);
  EXPECT_TEXT(
      source_anchor.get_span().caculate_text(*source),
      "/// Tetrodotoxin\n"
      "/// Copyright (c) 2023-present Matt Kaes and contributors\n"
      "//\n"
      "// Library source acceptance.\n"
      "dialect : Library;"_view);
  const Abstract& count_identity =
      source_type.resolve_concept("CountAlias"_view);
  const Abstract& mode_identity = source_type.resolve_concept("Mode"_view);
  const Abstract& packet_identity = source_type.resolve_concept("Packet"_view);
  const Abstract& session_identity =
      source_type.resolve_concept("Session"_view);
  ASSERT(count_identity.is<Alias>());
  ASSERT(mode_identity.is<Language::Types::Enumeration>());
  ASSERT(packet_identity.is<Language::Types::Structure>());
  ASSERT(session_identity.is<Language::Types::Object>());
  const auto& count_alias = static_cast<const Alias&>(count_identity);
  const auto& mode =
      static_cast<const Language::Types::Enumeration&>(mode_identity);
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  const auto& session =
      static_cast<const Language::Types::Object&>(session_identity);

  auto callables = source_type.get_callables();
  auto callable = callables.begin();
  ASSERT(callable != callables.end());
  ASSERT((*callable).get().is<Language::Function>());
  const auto& exported =
      static_cast<const Language::Function&>((*callable).get());
  ++callable;
  EXPECT(callable == callables.end());
  EXPECT(&count_alias.resolve() == &monograph.resolve_concept("U64"_view));
  ASSERT_EQ(mode.get_definition().get_attributes().get_size(), Count(1));
  EXPECT_TEXT(
      mode.get_definition().get_attributes().get_data()[0].get_key(),
      "presentation"_view);

  auto cases = mode.get_cases();
  ASSERT_EQ(cases.get_size(), Count(2));
  EXPECT_TEXT(cases.get_data()[0].get().get_name(), "idle"_view);
  EXPECT_TEXT(cases.get_data()[1].get().get_name(), "ready"_view);

  const Abstract& nested = packet.resolve_concept("Nested"_view);
  ASSERT(nested.is<Language::Types::Structure>());
  EXPECT_TEXT(
      packet.get_definition().get_attributes().get_data()[0].get_key(),
      "value_type"_view);
  const auto& nested_structure =
      static_cast<const Language::Types::Structure&>(nested);
  EXPECT_TEXT(
      nested_structure.get_definition()
          .get_attributes()
          .get_data()[0]
          .get_key(),
      "nested_type"_view);
  auto packet_field_identity = packet.get_layout().get_abstract(0);
  ASSERT(packet_field_identity);
  const auto& packet_field =
      static_cast<const Language::Field&>(*packet_field_identity);
  EXPECT_TEXT(packet_field.get_name(), "nested"_view);
  EXPECT(&packet_field.get_type() == &nested);
  EXPECT_TEXT(
      packet_field.get_definition().get_attributes().get_data()[0].get_key(),
      "member"_view);

  auto id_identity = session.get_layout().get_abstract(0);
  auto ready_identity = session.get_layout().get_abstract(1);
  ASSERT(id_identity);
  ASSERT(ready_identity);
  const auto& id_field = static_cast<const Language::Field&>(*id_identity);
  const auto& ready_field =
      static_cast<const Language::Field&>(*ready_identity);
  EXPECT_TEXT(id_field.get_name(), "id"_view);
  EXPECT_TEXT(ready_field.get_name(), "ready"_view);
  EXPECT(&id_field.get_type() == &monograph.resolve_concept("U64"_view));
  EXPECT_TEXT(
      session.get_definition().get_attributes().get_data()[0].get_key(),
      "reference_type"_view);
  EXPECT_TEXT(
      id_field.get_definition().get_attributes().get_data()[0].get_key(),
      "identity"_view);

  const Abstract& source_field_identity =
      source_type.resolve_concept("session"_view);
  ASSERT(source_field_identity.is<Language::Field>());
  const auto& source_field =
      static_cast<const Language::Field&>(source_field_identity);
  auto initializer = source_field.get_initializer();
  ASSERT(
      initializer &&
      initializer->is_identity<Language::Expressions::Initializer>());
  const auto& object_initializer =
      static_cast<const Language::Expressions::Initializer&>(*initializer);
  EXPECT(&object_initializer.get_type() == &session);
  auto initializer_attributes = source_field.get_definition().get_attributes();
  ASSERT_EQ(initializer_attributes.get_size(), Count(2));
  EXPECT_TEXT(
      initializer_attributes.get_data()[0].get_key(), "initializer"_view);
  EXPECT_TEXT(initializer_attributes.get_data()[1].get_key(), "abi"_view);

  auto attributes = exported.get_definition().get_attributes();
  ASSERT_EQ(attributes.get_size(), Count(4));
  EXPECT_TEXT(attributes.get_data()[0].get_key(), "abi"_view);
  EXPECT_TEXT(attributes.get_data()[1].get_key(), "symbol"_view);
  const View::Bytes* symbol =
      attributes.get_data()[1].get_value().find<View::Bytes>();
  ASSERT(symbol);
  EXPECT_TEXT(*symbol, "source_acceptance"_view);
  EXPECT_TEXT(attributes.get_data()[2].get_key(), "tooling"_view);
  EXPECT_TEXT(attributes.get_data()[3].get_key(), "tooling"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, slice_acceptance) {
  static constexpr View::Bytes path =
      "validation/data/ttx/library/value_acceptance.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Tetrodotoxin::Library::Dialect workspace_toolchain_library;

  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);

  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto interpreted =
      workspace.interpret_source(errors, "ValueAcceptance"_view, path, *source);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  EXPECT(&workspace.resolve_concept("ValueAcceptance"_view) == &monograph);

  const auto& source_type = monograph.get_source();
  const Abstract& packet_identity = source_type.resolve_concept("Packet"_view);
  const Abstract& pair_identity = source_type.resolve_concept("Pair"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  ASSERT(pair_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  const auto& pair =
      static_cast<const Language::Types::Structure&>(pair_identity);
  auto width = find_field(packet, "width"_view);
  auto height = find_field(packet, "height"_view);
  ASSERT(width);
  ASSERT(height);

  auto sum = find_field(source_type, "sum"_view);
  ASSERT(sum && sum->get_initializer());
  ASSERT(sum->get_initializer()->is_identity<Language::Operations::Add>());
  const auto& sum_expression =
      static_cast<const Language::Expression&>(*sum->get_initializer());
  auto folded_sum = sum_expression.get_folded();
  ASSERT(
      folded_sum && folded_sum->is_identity<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*folded_sum)
          .get_value(),
      U64(5));

  auto constant_offset = find_field(source_type, "constant_offset"_view);
  ASSERT(constant_offset && constant_offset->get_initializer());
  const auto& constant_offset_expression =
      static_cast<const Language::Expression&>(
          *constant_offset->get_initializer());
  auto folded_offset = constant_offset_expression.get_folded();
  ASSERT(
      folded_offset &&
      folded_offset->is_identity<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*folded_offset)
          .get_value(),
      U64(13));

  auto sequence = find_field(source_type, "sequence"_view);
  auto selected_byte = find_field(source_type, "selected_byte"_view);
  auto missing_byte = find_field(source_type, "missing_byte"_view);
  auto selected_slice = find_field(source_type, "selected_slice"_view);
  auto missing_slice = find_field(source_type, "missing_slice"_view);
  auto constant_slice = find_field(source_type, "constant_slice"_view);
  ASSERT(sequence);
  ASSERT(selected_byte);
  ASSERT(missing_byte);
  ASSERT(selected_slice);
  ASSERT(missing_slice);
  ASSERT(constant_slice && constant_slice->get_initializer());
  ASSERT(sequence->get_type().is<Language::Types::Range>());
  const auto& range =
      static_cast<const Language::Types::Range&>(sequence->get_type());
  EXPECT(&range.get_element_type() == &monograph.resolve_concept("U64"_view));
  EXPECT(&selected_byte->get_type() == &monograph.resolve_concept("U8"_view));
  EXPECT(&missing_byte->get_type() == &monograph.resolve_concept("U8"_view));
  ASSERT(
      missing_byte->get_initializer() &&
      missing_byte->get_initializer()->is_identity<Language::Access::Slice>());
  const auto& default_expression = static_cast<const Language::Expression&>(
      *missing_byte->get_initializer());
  auto folded_default = default_expression.get_folded();
  ASSERT(
      folded_default &&
      folded_default->is_identity<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*folded_default)
          .get_value(),
      U64(0));
  ASSERT(selected_slice->get_type().is<Language::Types::Fixed>());
  EXPECT(&selected_slice->get_type() == &missing_slice->get_type());
  const auto& slice_type =
      static_cast<const Language::Types::Fixed&>(selected_slice->get_type());
  EXPECT(
      &slice_type.get_element_type() == &monograph.resolve_concept("U8"_view));
  EXPECT_EQ(slice_type.get_extent(), U64(2));

  const auto& constant_slice_expression =
      static_cast<const Language::Expression&>(
          *constant_slice->get_initializer());
  auto folded_constant_slice = constant_slice_expression.get_folded();
  ASSERT(folded_constant_slice);
  ASSERT_EQ(folded_constant_slice->get_layout().get_size(), Count(2));
  auto constant_slice_first =
      folded_constant_slice->get_layout().get_abstract(0);
  auto constant_slice_second =
      folded_constant_slice->get_layout().get_abstract(1);
  ASSERT(
      constant_slice_first &&
      constant_slice_first->is<Language::Constants::Unsigned>());
  ASSERT(
      constant_slice_second &&
      constant_slice_second->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*constant_slice_first)
          .get_value(),
      U64(0x0D));
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*constant_slice_second)
          .get_value(),
      U64(0x0E));

  auto called = find_field(source_type, "called"_view);
  auto addressed = find_field(source_type, "addressed"_view);
  auto self_called = find_field(source_type, "self_called"_view);
  ASSERT(called && called->get_initializer());
  ASSERT(addressed && addressed->get_initializer());
  ASSERT(self_called && self_called->get_initializer());
  ASSERT(called->get_initializer()->is_identity<Language::Access::Call>());
  ASSERT(
      addressed->get_initializer()->is_identity<Language::Access::Address>());
  ASSERT(self_called->get_initializer()->is_identity<Language::Access::Call>());
  const auto& address = static_cast<const Language::Access::Address&>(
      *addressed->get_initializer());
  const auto& self_call = static_cast<const Language::Access::Call&>(
      *self_called->get_initializer());
  EXPECT(address.get_receiver().get_result().is<Addressable>());
  EXPECT(&address.get_result() == &*width);
  ASSERT(self_call.get_callable());
  EXPECT(self_call.get_callable()->is_type_bound(packet));

  auto empty = find_function(source_type, "empty"_view);
  auto single = find_field(source_type, "single"_view);
  auto reordered = find_field(source_type, "reordered"_view);
  ASSERT(empty);
  auto empty_return = find_return(*empty);
  ASSERT(empty_return);
  EXPECT_TEXT(
      empty_return->get_anchor().get_span().caculate_text(*source),
      "return packet.[];"_view);
  EXPECT(empty->get_results().is_empty());
  ASSERT(single && single->get_initializer());
  ASSERT(reordered && reordered->get_initializer());
  ASSERT(single->get_initializer()->is_identity<Language::Access::Swizzle>());
  ASSERT(
      reordered->get_initializer()->is_identity<Language::Access::Swizzle>());
  const auto& single_swizzle =
      static_cast<const Language::Access::Swizzle&>(*single->get_initializer());
  const auto& reordered_swizzle = static_cast<const Language::Access::Swizzle&>(
      *reordered->get_initializer());
  ASSERT_EQ(single_swizzle.get_layout().get_size(), Count(1));
  auto single_output = single_swizzle.get_layout().get_abstract(0);
  ASSERT(single_output && single_output->is<Language::Access::Address>());
  EXPECT(
      &static_cast<const Language::Access::Address&>(*single_output)
           .get_result() == &*width);
  ASSERT_EQ(reordered_swizzle.get_layout().get_size(), Count(2));
  auto first_output = reordered_swizzle.get_layout().get_abstract(0);
  auto second_output = reordered_swizzle.get_layout().get_abstract(1);
  ASSERT(first_output && first_output->is<Language::Access::Address>());
  ASSERT(second_output && second_output->is<Language::Access::Address>());
  EXPECT(
      &static_cast<const Language::Access::Address&>(*first_output)
           .get_result() == &*height);
  EXPECT(
      &static_cast<const Language::Access::Address&>(*second_output)
           .get_result() == &*width);
  EXPECT(reordered_swizzle.fits(pair));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, executable_source) {
  static constexpr View::Bytes path =
      "validation/data/ttx/products/runtime/runtime.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Tetrodotoxin::Library::Dialect workspace_toolchain_library;

  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);

  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "ExecutableAcceptance"_view, path, *source);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  EXPECT(&workspace.resolve_concept("ExecutableAcceptance"_view) == &monograph);

  Dialect archive_dialect;
  auto complete = archive_dialect.encode(monograph);
  auto complete_again = archive_dialect.encode(monograph);
  ASSERT(complete && complete_again);
  EXPECT(*complete == *complete_again);
  ASSERT(complete->get_size() >= 8);
  EXPECT_EQ((*complete)[4], U8(2));
  EXPECT_EQ((*complete)[6], U8(0));
  Allocator::Arena restored_arena;
  auto decoded = archive_dialect.decode(restored_arena, *complete, workspace);
  auto restored = decoded ? decoded->select<Language::Monograph>()
                          : Option<Language::Monograph&>();
  ASSERT(restored);
  ASSERT(restored->link_restored());
  ASSERT(restored->finalize_restored());
  auto restored_library = restored->select<Language::Monograph>();
  ASSERT(restored_library);
  auto restored_execute = find_function(
      restored_library->get_source(), "executable_acceptance"_view);
  ASSERT(restored_execute);
  EXPECT_NOT(restored_execute->get_body());

  auto restored_present =
      find_field(restored_library->get_source(), "present"_view);
  ASSERT(restored_present);
  auto restored_constant = restored_present->get_constant();
  auto restored_option =
      restored_constant
          ? restored_constant->select_identity<Language::Constants::Option>()
          : Option<Language::Constants::Option&>();
  ASSERT(restored_option);
  auto restored_payload = restored_option->get_payload();
  auto restored_value =
      restored_payload
          ? restored_payload->select_identity<Language::Constants::Unsigned>()
          : Option<const Language::Constants::Unsigned&>();
  ASSERT(restored_value);
  EXPECT_EQ(restored_value->get_value(), U64(5));

  const auto& source_type = monograph.get_source();
  auto execute = find_function(source_type, "executable_acceptance"_view);
  ASSERT(execute && execute->get_body());
  const auto& parameters = execute->get_parameters();
  auto flag = parameters.get_abstract(0);
  ASSERT(flag);

  // Block order is the executable source order. Keeping the complete owner
  // inventory flat here also proves the Call remains the statement itself.
  auto statements = execute->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(7));
  ASSERT(statements.get_data()[0].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[1].get_root().is<Language::Flow::Local>());
  ASSERT(statements.get_data()[2].get_root().is<Language::Access::Call>());
  ASSERT(statements.get_data()[3].get_root().is<Language::Flow::Branch>());
  ASSERT(statements.get_data()[4].get_root().is<Language::Flow::Branch>());
  ASSERT(statements.get_data()[5].get_root().is<Language::Flow::RangeLoop>());
  ASSERT(statements.get_data()[6].get_root().is<Language::Flow::Match>());

  const auto& total = static_cast<const Language::Flow::Local&>(
      statements.get_data()[0].get_root());
  const auto& one = static_cast<const Language::Flow::Local&>(
      statements.get_data()[1].get_root());
  EXPECT_TEXT(total.get_name(), "total"_view);
  EXPECT_TEXT(one.get_name(), "one"_view);
  EXPECT(total.get_writability() == Language::Writability::Full);
  EXPECT(one.get_writability() == Language::Writability::Constant);

  const auto& call = static_cast<const Language::Access::Call&>(
      statements.get_data()[2].get_root());
  ASSERT(call.get_callable());
  EXPECT_TEXT(call.get_callable()->get_name(), "tick"_view);
  EXPECT_NOT(call.get_folded());

  const auto& conditional = static_cast<const Language::Flow::Branch&>(
      statements.get_data()[3].get_root());
  EXPECT(conditional.get_kind() == Language::Flow::Branch::Kind::If);
  ASSERT_EQ(conditional.get_body().get_statements().get_size(), Count(1));
  ASSERT(conditional.get_body()
             .get_statements()
             .get_data()[0]
             .get_root()
             .is<Language::Operations::AddAssignment>());
  ASSERT(conditional.get_alternate());
  auto alternate =
      conditional.get_alternate()->get_root().select<Language::Flow::Block>();
  ASSERT(alternate);
  ASSERT_EQ(alternate->get_statements().get_size(), Count(1));
  ASSERT(alternate->get_statements()
             .get_data()[0]
             .get_root()
             .is<Language::Operations::Assignment>());

  const auto& while_loop = static_cast<const Language::Flow::Branch&>(
      statements.get_data()[4].get_root());
  EXPECT(while_loop.get_kind() == Language::Flow::Branch::Kind::While);
  auto while_statements = while_loop.get_body().get_statements();
  ASSERT_EQ(while_statements.get_size(), Count(2));
  ASSERT(while_statements.get_data()[0]
             .get_root()
             .is<Language::Operations::AddAssignment>());
  const auto& broken = static_cast<const Language::Flow::LoopControl&>(
      while_statements.get_data()[1].get_root());
  EXPECT(broken.get_kind() == Language::Flow::LoopControl::Kind::Break);
  EXPECT(&broken.get_target() == &while_loop);

  // The nested Branch contributes lexical scope but does not replace the
  // RangeLoop selected by continue. The retained edge stays on the real loop.
  const auto& range_loop = static_cast<const Language::Flow::RangeLoop&>(
      statements.get_data()[5].get_root());
  auto range_statements = range_loop.get_body().get_statements();
  ASSERT_EQ(range_statements.get_size(), Count(2));
  const auto& range_branch = static_cast<const Language::Flow::Branch&>(
      range_statements.get_data()[0].get_root());
  const auto& continued = static_cast<const Language::Flow::LoopControl&>(
      range_branch.get_body().get_statements().get_data()[0].get_root());
  EXPECT(continued.get_kind() == Language::Flow::LoopControl::Kind::Continue);
  EXPECT(&continued.get_target() == &range_loop);
  ASSERT(range_statements.get_data()[1]
             .get_root()
             .is<Language::Operations::AddAssignment>());

  // Exhaustive Flag cases make Match the Function terminal. Each Return stays
  // inside its real case Block rather than becoming a copied result edge.
  const auto& match = static_cast<const Language::Flow::Match&>(
      statements.get_data()[6].get_root());
  EXPECT(&match.get_input().get_result() == &*flag);
  ASSERT_EQ(match.get_case_count(), Count(2));
  auto first_case = match.get_case_constant(0);
  auto second_case = match.get_case_constant(1);
  ASSERT(first_case && first_case->is<Language::Constants::Flag>());
  ASSERT(second_case && second_case->is<Language::Constants::Flag>());
  EXPECT_NOT(
      static_cast<const Language::Constants::Flag&>(*first_case).get_value());
  EXPECT(
      static_cast<const Language::Constants::Flag&>(*second_case).get_value());
  ASSERT(match.get_case_body(0));
  ASSERT(match.get_case_body(1));
  ASSERT(match.get_case_body(0)
             ->get_statements()
             .get_data()[0]
             .get_root()
             .is<Language::Flow::Return>());
  ASSERT(match.get_case_body(1)
             ->get_statements()
             .get_data()[0]
             .get_root()
             .is<Language::Flow::Return>());
  EXPECT_NOT(match.get_default());
  EXPECT_NOT(match.reaches_next_statement());
  EXPECT_NOT(execute->get_body()->reaches_next_statement());

  const Abstract& retained_match = match;
  Allocator::Arena repeated_domain;
  Tokenizer repeated_tokenizer(
      repeated_domain, *source, "executable-acceptance.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations repeated_associations(
      repeated_tokenizer.get_arena());
  Cursor repeated_cursor(repeated_tokenizer, errors, repeated_associations);
  ASSERT(monograph.link(repeated_cursor));
  ASSERT(monograph.finalize(repeated_cursor));
  EXPECT(
      &execute->get_body()->get_statements().get_data()[6].get_root() ==
      &retained_match);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, resource_slice) {
  static constexpr View::Bytes source =
      "public const offset : U64 = 1;\n"
      "public const size : U64 = 2;\n"
      "private folded : Fixed[U8, 2] =\n"
      "  $[resource/hello.txt]:[\n"
      "    source.offset + 12,\n"
      "    source.size\n"
      "  ];\n"
      "private extract : func = [] -> [] {\n"
      "  const embedded := $[resource/greeting.txt];\n"
      "  const hello : Fixed[U8, 5] = embedded:[0, 5];\n"
      "  return;\n"
      "}"_view;
  Allocator::Arena arena;
  ResourceRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "resource-slice-fold.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto interpreted_owner =
      interpret_library_source(arena, dialect, cursor, registry);
  auto interpreted = select_library_monograph(interpreted_owner);
  ASSERT(interpreted);
  auto& monograph = *interpreted;
  ASSERT(monograph.link(cursor));
  ASSERT(monograph.finalize(cursor));
  auto folded = find_field(monograph.get_source(), "folded"_view);
  ASSERT(folded && folded->get_initializer());
  const auto& slice =
      static_cast<const Language::Expression&>(*folded->get_initializer());
  auto constants = slice.get_folded();
  ASSERT(constants);
  ASSERT_EQ(constants->get_layout().get_size(), Count(2));
  auto first = constants->get_layout().get_abstract(0);
  auto second = constants->get_layout().get_abstract(1);
  ASSERT(first && first->is<Language::Constants::Unsigned>());
  ASSERT(second && second->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*first).get_value(),
      U64('D'));
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*second).get_value(),
      U64('E'));

  auto extract = find_function(monograph.get_source(), "extract"_view);
  ASSERT(extract && extract->get_body());
  auto statements = extract->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(3));
  auto hello =
      statements.get_data()[1].get_root().select<Language::Flow::Local>();
  ASSERT(hello);
  auto hello_value = hello->get_constant();
  ASSERT(hello_value);
  auto hello_bytes = hello_value->select_identity<Language::Constants::Bytes>();
  ASSERT(hello_bytes);
  EXPECT_TEXT(hello_bytes->get_value(), "Hello"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, const_field_access) {
  static constexpr View::Bytes source =
      "public Packet : struct {\n"
      "  public const offset := base;\n"
      "  public const base : U64 = 1;\n"
      "  public state value : U64;\n"
      "}\n"
      "private packet : Packet;\n"
      "private from_type := Packet.offset + 12;"_view;
  Allocator::Arena arena;
  ResourceRegistry registry;
  Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, source, "instance-const-fold.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto interpreted_owner =
      interpret_library_source(arena, dialect, cursor, registry);
  auto interpreted = select_library_monograph(interpreted_owner);
  ASSERT(interpreted);
  auto& monograph = *interpreted;
  ASSERT(monograph.link(cursor));
  const Abstract& packet_identity = monograph.resolve_concept("Packet"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  ASSERT_EQ(packet.get_layout().get_size(), Count(1));
  auto instance_entry = packet.get_layout().get_abstract(0);
  ASSERT(instance_entry);
  EXPECT_TEXT(instance_entry->get_name(), "value"_view);
  auto offset = find_field(packet, "offset"_view);
  ASSERT(offset);
  auto linked_constant = offset->get_constant();
  ASSERT(linked_constant);
  ASSERT(linked_constant->is_identity<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*linked_constant)
          .get_value(),
      U64(1));

  ASSERT(monograph.finalize(cursor));

  auto from_type = find_field(monograph.get_source(), "from_type"_view);
  ASSERT(from_type && from_type->get_initializer());
  auto type_expression =
      from_type->get_initializer()->select_identity<Language::Expression>();
  ASSERT(type_expression);
  auto type_constant = type_expression->get_folded();
  ASSERT(
      type_constant &&
      type_constant->is_identity<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*type_constant)
          .get_value(),
      U64(13));
  EXPECT(errors.is_empty());
}
