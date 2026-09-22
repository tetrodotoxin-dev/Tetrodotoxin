// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/object.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "ObjectTest"_view, "object.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_source(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return !monograph && !errors.is_empty();
}

static Harness ObjectTests = {
  .name = "Tetrodotoxin::Library::Language::Types::Object"_view,
};

PERIMORTEM_UNIT_TEST(ObjectTests, field_writability) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "public Session : object {\n"
      "  public state open : Bool;\n"
      "  private closed : Bool;\n"
      "  expose state observed : Bool = false;\n"
      "  private state hidden_state : Bool = false;\n"
      "  public const fixed : Bool = false;\n"
      "  private const hidden_const : Bool = false;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& selected = monograph->resolve_concept("Session"_view);
  ASSERT(selected.is<Language::Types::Object>());
  const auto& object = static_cast<const Language::Types::Object&>(selected);
  auto fields = object.get_addressables();

  // Every authored mode remains a policy on the real Field identity rather
  // than creating another mutable binding.
  auto field = fields.begin();
  ASSERT(field != fields.end());
  const auto& open = static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& closed = static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& observed = static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& hidden_state =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& fixed = static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& hidden_const =
      static_cast<const Language::Field&>((*field).get());
  EXPECT(open.get_writability() == Language::Writability::Internal);
  EXPECT(closed.get_writability() == Language::Writability::Full);
  EXPECT(observed.get_writability() == Language::Writability::Internal);
  EXPECT(hidden_state.get_writability() == Language::Writability::Internal);
  EXPECT(fixed.get_writability() == Language::Writability::Constant);
  EXPECT(hidden_const.get_writability() == Language::Writability::Constant);
  ASSERT_EQ(object.get_layout().get_size(), Count(3));
  auto first_layout_entry = object.get_layout().get_abstract(0);
  auto second_layout_entry = object.get_layout().get_abstract(1);
  auto third_layout_entry = object.get_layout().get_abstract(2);
  ASSERT(first_layout_entry);
  ASSERT(second_layout_entry);
  ASSERT(third_layout_entry);
  EXPECT(&*first_layout_entry == &open);
  EXPECT(&*second_layout_entry == &observed);
  EXPECT(&*third_layout_entry == &hidden_state);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, collision_domain) {
  static constexpr Static::Vector<View::Bytes, 2> accepted = {{
    "// Object test.\ndialect : Library; public Session : object { public value : func = [] -> [] {} private state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { private state value : Bool = false; public value : func = [] -> [] {} }"_view,
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

  static constexpr Static::Vector<View::Bytes, 6> rejected = {{
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool = false; private state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { public value : func = [] -> [] {} private value : func = [] -> [] {} }"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : object {}"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : struct {}"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : enum[U8] {}"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : func = [] -> [] {}"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    EXPECT(rejects_source(rejected[i]));
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, malformed_grammar) {
  static constexpr Static::Vector<View::Bytes, 7> sources = {{
    "// Object test.\ndialect : Library; public Session object {}"_view,
    "// Object test.\ndialect : Library; public Session : managed {}"_view,
    "// Object test.\ndialect : Library; public Session : object { state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool = false }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state Value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool = false;"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_source(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, private_exposure) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Object test.\ndialect : Library; private Hidden : object { private state value : Bool = false; } public reveal : func = [.hidden : Hidden] -> [] {}"_view,
    "// Object test.\ndialect : Library; private Hidden : object { private state value : Bool = false; } public Holder : struct { public state hidden : Hidden; }"_view,
    "// Object test.\ndialect : Library; private Hidden : object { private state value : Bool = false; } public Holder : object { public state hidden : Hidden; }"_view,
    "// Object test.\ndialect : Library; private Hidden : object { private state value : Bool = false; } public Holder : object { public reveal : func = [.hidden : Hidden] -> Hidden { return hidden; } }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_source(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, private_surface) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "private Hidden : object { private state value : Bool = false; }\n"
      "public Holder : object {\n"
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
  ASSERT((*types).get().is<Language::Types::Object>());
  const Abstract& hidden = (*types).get();
  ASSERT(source_callables != source_callables.end());
  ASSERT((*source_callables).get().is<Language::Function>());
  const auto& root =
      static_cast<const Language::Function&>((*source_callables).get());
  const Abstract& holder = monograph->resolve_concept("Holder"_view);
  EXPECT(hidden.is<Language::Types::Object>());
  ASSERT(holder.is<Language::Types::Object>());
  EXPECT(&monograph->resolve_concept("Hidden"_view) == &Unknown::get_unknown());
  EXPECT(&holder.resolve_concept("hidden"_view) == &Unknown::get_unknown());
  EXPECT(&holder.resolve_concept("reveal"_view) == &Unknown::get_unknown());
  EXPECT(&monograph->resolve_concept("root"_view) == &Unknown::get_unknown());
  const auto& holder_object =
      static_cast<const Language::Types::Object&>(holder);
  auto fields = holder_object.get_addressables();
  auto callables = holder_object.get_callables();
  ASSERT(fields != fields.end());
  ASSERT(callables != callables.end());
  ASSERT((*callables).get().is<Language::Function>());
  const auto& reveal =
      static_cast<const Language::Function&>((*callables).get());
  EXPECT(&reveal.resolve_concept("hidden"_view) == &Unknown::get_unknown());
  EXPECT(&reveal.resolve_concept("Hidden"_view) == &hidden);
  EXPECT(&root.resolve_concept("Hidden"_view) == &hidden);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, member_rejections) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Bad initializer.\ndialect : Library;\npublic Session : object { private state value : U8 = false; }"_view,
    "// Unresolved initializer.\ndialect : Library;\npublic Session : object { expose state value : Bool = missing; }"_view,
    "// Missing Type.\ndialect : Library;\npublic Session : object { private state value : Missing = false; }"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    Tetrodotoxin::Library::Dialect workspace_toolchain_library;
    auto workspace_toolchain =
        Validation::create_library_toolchain(workspace_toolchain_library);
    Workspace workspace(*workspace_toolchain);
    Errors errors;
    auto monograph = interpret(workspace, errors, sources[index]);
    EXPECT_NOT(monograph);
    EXPECT(retains_library_source(workspace, "ObjectTest"_view));
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, inferred_identity) {
  static constexpr View::Bytes source =
      "// Object inference test.\n"
      "dialect : Library;\n"
      "public Child : object { private state value : Bool = false; }\n"
      "public Holder : object {\n"
      "  private child : Child;\n"
      "  private copy := child;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  const Abstract& child_identity = monograph->resolve_concept("Child"_view);
  const Abstract& holder_identity = monograph->resolve_concept("Holder"_view);
  ASSERT(child_identity.is<Language::Types::Object>());
  ASSERT(holder_identity.is<Language::Types::Object>());
  const auto& child =
      static_cast<const Language::Types::Object&>(child_identity);
  const auto& holder =
      static_cast<const Language::Types::Object&>(holder_identity);

  auto fields = holder.get_addressables();
  ASSERT(fields != fields.end());
  const auto& child_field =
      static_cast<const Language::Field&>((*fields).get());
  ++fields;
  ASSERT(fields != fields.end());
  const auto& copy_field = static_cast<const Language::Field&>((*fields).get());
  EXPECT(&child_field.get_type() == &child);
  EXPECT(&copy_field.get_type() == &child);
  ASSERT(copy_field.get_initializer());
  ASSERT(copy_field.get_initializer()
             ->is_identity<Language::Expressions::Identifier>());
  const auto& identifier =
      static_cast<const Language::Expressions::Identifier&>(
          *copy_field.get_initializer());
  EXPECT(&identifier.get_result() == &child_field);
  EXPECT(errors.is_empty());
}
