// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/type_reference.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/u64.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "tetrodotoxin/source/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness LibraryTypeReference = {
  .name = "Tetrodotoxin::Library::Language::TypeReference"_view,
};

class RouteType : public Type {
 public:
  TTX_CONTRACT(RouteType, Type);
  TTX_NAME("Second"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto resolve_concept(View::Bytes) const
      -> const Abstract& override {
    return Unknown::get_unknown();
  }
};

class RouteContext : public Abstract {
 public:
  RouteContext(View::Bytes name, View::Bytes child_name, const Abstract& child)
      : name(name), child_name(child_name), child(child) {}

  TTX_CONTRACT(RouteContext, Abstract);
  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto resolve_concept(View::Bytes route) const
      -> const Abstract& override {
    return route == child_name ? child : Unknown::get_unknown();
  }

 private:
  View::Bytes name;
  View::Bytes child_name;
  const Abstract& child;
};

static auto interpret(
    Workspace& workspace,
    Errors& errors,
    View::Bytes semantic_name,
    View::Bytes source) -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, semantic_name, "type-reference.ttx"_view, source);
  BAIL_IF(!interpreted || !interpreted->is<Language::Monograph>());

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto find_field(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Field&> {
  for (const Reference<Abstract>& binding : composite.get_addressables()) {
    if (binding.get().get_name() == name &&
        binding.get().is<Language::Field>()) {
      return static_cast<const Language::Field&>(binding.get());
    }
  }

  return {};
}

static auto select_structure(
    const Language::Types::Source& source,
    View::Bytes name) -> Option<const Language::Types::Structure&> {
  const Abstract& selected = source.resolve_concept(name);
  BAIL_IF(!selected.is<Language::Types::Structure>());

  return static_cast<const Language::Types::Structure&>(selected);
}

PERIMORTEM_UNIT_TEST(LibraryTypeReference, segment_queries) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "First::Second"_view, "route.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto reference = Interpreter::TypeReference::parse_route(cursor);
  ASSERT(reference);

  RouteType terminal;
  RouteContext first("First"_view, "Second"_view, terminal);
  RouteContext root("Root"_view, "First"_view, first);
  Option<const Abstract&> selected;
  reference->resolve(root).visit(
      [&](const Abstract& resolved) { selected = resolved; },
      [](const Language::TypeReference::Failure&) {});
  EXPECT(selected && &*selected == &terminal);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryTypeReference, explicit_package_alias) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "Math::U64"_view, "route.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto reference = Interpreter::TypeReference::parse_route(cursor);
  ASSERT(reference);

  Language::Types::U64 terminal;
  Alias exported_alias("U64"_view, terminal);
  RouteContext package("Package"_view, "U64"_view, exported_alias);
  Alias package_alias("Math"_view, package);
  RouteContext root("Root"_view, "Math"_view, package_alias);
  Option<const Abstract&> selected;
  reference->resolve(root).visit(
      [&](const Abstract& resolved) { selected = resolved; },
      [](const Language::TypeReference::Failure&) {});
  EXPECT(selected && &*selected == &terminal);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryTypeReference, generic_identity) {
  static constexpr View::Bytes source =
      "// Generic TypeReference test.\n"
      "dialect : Library;\n"
      "public Catalog : struct {\n"
      "  public values : Access[Later];\n"
      "  public nested : View[Fixed[U8, 4,],];\n"
      "  public repeated : View[Fixed[U8, 4]];\n"
      "}\n"
      "public Later : struct { public state ready : Bool; }\n"
      "public Node : struct {\n"
      "  public state ready : Bool;\n"
      "  public children : View[Node];\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph =
      interpret(workspace, errors, "GenericTypeReference"_view, source);
  ASSERT(monograph);
  EXPECT(errors.is_empty());

  const auto& root = monograph->get_source();
  auto catalog = select_structure(root, "Catalog"_view);
  auto later = select_structure(root, "Later"_view);
  auto node = select_structure(root, "Node"_view);
  ASSERT(catalog);
  ASSERT(later);
  ASSERT(node);

  auto values = find_field(*catalog, "values"_view);
  auto nested = find_field(*catalog, "nested"_view);
  auto repeated = find_field(*catalog, "repeated"_view);
  auto children = find_field(*node, "children"_view);
  ASSERT(values);
  ASSERT(nested);
  ASSERT(repeated);
  ASSERT(children);

  auto access = values->get_type().select<Language::Types::Access>();
  auto nested_view = nested->get_type().select<Language::Types::View>();
  auto repeated_view = repeated->get_type().select<Language::Types::View>();
  auto child_view = children->get_type().select<Language::Types::View>();
  ASSERT(access);
  ASSERT(nested_view);
  ASSERT(repeated_view);
  ASSERT(child_view);
  EXPECT(&access->get_element_type() == &*later);
  EXPECT(&nested->get_type() == &repeated->get_type());
  EXPECT(&child_view->get_element_type() == &*node);

  auto fixed = nested_view->get_element_type().select<Language::Types::Fixed>();
  ASSERT(fixed);
  EXPECT(&fixed->get_element_type() == &monograph->resolve_concept("U8"_view));
  EXPECT_EQ(fixed->get_extent(), U64(4));

  auto values_type = values->get_type().select<Type>();
  auto nested_type = nested->get_type().select<Type>();
  auto children_type = children->get_type().select<Type>();
  ASSERT(values_type && nested_type && children_type);

  // Repeating completion observes the same Generic owned materializations
  // and the exact Types selected by the first pass.
  Allocator::Arena repeat_domain;
  Tokenizer repeat_tokenizer(repeat_domain, source, "type-reference.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations repeat_associations(repeat_tokenizer.get_arena());
  Cursor repeat_cursor(repeat_tokenizer, errors, repeat_associations);
  ASSERT(monograph->link(repeat_cursor));
  catalog = select_structure(root, "Catalog"_view);
  node = select_structure(root, "Node"_view);
  ASSERT(catalog);
  ASSERT(node);
  values = find_field(*catalog, "values"_view);
  nested = find_field(*catalog, "nested"_view);
  children = find_field(*node, "children"_view);
  ASSERT(values);
  ASSERT(nested);
  ASSERT(children);
  EXPECT(&values->get_type() == &*values_type);
  EXPECT(&nested->get_type() == &*nested_type);
  EXPECT(&children->get_type() == &*children_type);

  ASSERT(monograph->finalize(repeat_cursor));
  EXPECT(errors.is_empty());
  EXPECT(
      &workspace.resolve_concept("GenericTypeReference"_view) == &*monograph);
}

PERIMORTEM_UNIT_TEST(LibraryTypeReference, generic_aliases) {
  static constexpr View::Bytes source =
      "// Generic Alias TypeReference test.\n"
      "dialect : Library;\n"
      "public LocalView : alias = View[LaterAlias];\n"
      "public QualifiedAccess : alias = Access[Container::NestedAlias];\n"
      "public LaterAlias : alias = Later;\n"
      "public Container : struct {\n"
      "  public NestedAlias : alias = Later;\n"
      "}\n"
      "public Later : struct { public state ready : Bool; }"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph =
      interpret(workspace, errors, "GenericAliasTypeReference"_view, source);
  ASSERT(monograph);
  EXPECT(errors.is_empty());

  const auto& root = monograph->get_source();
  auto later = select_structure(root, "Later"_view);
  ASSERT(later);

  const Abstract& local_alias = root.resolve_concept("LocalView"_view);
  const Abstract& qualified_alias =
      root.resolve_concept("QualifiedAccess"_view);
  ASSERT(local_alias.is<Tetrodotoxin::Source::Alias>());
  ASSERT(qualified_alias.is<Tetrodotoxin::Source::Alias>());

  auto local_view = local_alias.resolve().select<Language::Types::View>();
  auto qualified_access =
      qualified_alias.resolve().select<Language::Types::Access>();
  ASSERT(local_view);
  ASSERT(qualified_access);
  EXPECT(&local_view->get_element_type() == &*later);
  EXPECT(&qualified_access->get_element_type() == &*later);

  // Completing an Alias reached from a Generic argument does not publish a
  // second generated identity. The two distinct formulas retain one
  // canonical materialization each, regardless of the Alias chain used to
  // reach Later.
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryTypeReference, alias_cycle) {
  static constexpr View::Bytes source =
      "// Recursive Generic Alias rejection.\n"
      "dialect : Library;\n"
      "public First : alias = View[Second];\n"
      "public Second : alias = View[First];"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph =
      interpret(workspace, errors, "RecursiveGenericAlias"_view, source);
  EXPECT_NOT(monograph);
  ASSERT_EQ(errors.get_size(), Count(2));
  Allocator::Arena rendered;
  EXPECT(
      Algorithm::search(
          errors.render_message(rendered, 0),
          "Library route segment 1 did not resolve in its selected context."_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(LibraryTypeReference, generic_errors) {
  struct Rejection {
    View::Bytes semantic_name;
    View::Bytes source;
    View::Bytes route;
    View::Bytes message;
    View::Bytes focus;
    View::Bytes marker;
  };
  static constexpr Static::Vector<Rejection, 6> rejections = {{
    Rejection{
      "ConcreteTypeArguments"_view,
      "// Concrete Type argument rejection.\n"
      "dialect : Library;\n"
      "public invalid : U8[U8];"_view,
      "U8[U8]"_view,
      "Library Type arguments require a Generic at the route terminal."_view,
      {},
      {},
    },
    {
      "BareGeneric"_view,
      "// Bare Generic rejection.\n"
      "dialect : Library;\n"
      "public invalid : View;"_view,
      "View"_view,
      "Field Type route did not resolve to one stable Type."_view,
      {},
      {},
    },
    {
      "EmptyGenericArguments"_view,
      "// Empty Generic argument rejection.\n"
      "dialect : Library;\n"
      "public invalid : View[];"_view,
      "View[]"_view,
      "Library Generic application has the wrong number of arguments."_view,
      {},
      {},
    },
    {
      "WrongGenericCategory"_view,
      "// Generic category rejection.\n"
      "dialect : Library;\n"
      "public invalid : Fixed[U8, false];"_view,
      "Fixed[U8, false]"_view,
      "Library Generic argument 2 does not satisfy its parameter category."_view,
      "type-reference.ttx:3:28:"_view,
      "^----"_view,
    },
    {
      "RejectedGenericFormula"_view,
      "// Generic formula rejection.\n"
      "dialect : Library;\n"
      "public invalid : Fixed[U8, 0];"_view,
      "Fixed[U8, 0]"_view,
      "Library Generic rejected this argument combination."_view,
      {},
      {},
    },
    {
      "NestedGenericRoute"_view,
      "// Nested Generic route rejection.\n"
      "dialect : Library;\n"
      "public invalid : View[Missing];"_view,
      "View[Missing]"_view,
      "Library route segment 1 did not resolve in its selected context."_view,
      "type-reference.ttx:3:23:"_view,
      "^------"_view,
    },
  }};

  for (Count i = 0; i < rejections.get_size(); i++) {
    const Rejection& rejection = rejections.get_data()[i];
    Tetrodotoxin::Library::Dialect workspace_toolchain_library;
    auto workspace_toolchain =
        Validation::create_library_toolchain(workspace_toolchain_library);
    Workspace workspace(*workspace_toolchain);
    Errors errors;
    auto monograph =
        interpret(workspace, errors, rejection.semantic_name, rejection.source);
    EXPECT_NOT(monograph);
    ASSERT_EQ(errors.get_size(), Count(1));
    Allocator::Arena rendered;
    auto message = errors.render_message(rendered, 0);
    EXPECT(Algorithm::search(message, rejection.route) != Count(-1));
    EXPECT(Algorithm::search(message, rejection.message) != Count(-1));
    if (!rejection.focus.is_empty()) {
      EXPECT(Algorithm::search(message, rejection.focus) != Count(-1));
      EXPECT(Algorithm::search(message, rejection.marker) != Count(-1));
    }
  }
}
