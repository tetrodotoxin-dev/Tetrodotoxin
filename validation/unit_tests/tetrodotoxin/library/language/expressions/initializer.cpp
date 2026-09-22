// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/expressions/initializer.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
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
      errors, "InitializerTest"_view, "initializer.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_interpretation(
    View::Bytes source,
    View::Bytes diagnostic = {}) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  BAIL_IF(interpret(workspace, errors, source) || errors.is_empty());
  if (diagnostic.is_empty()) {
    return True;
  }

  Perimortem::Memory::Allocator::Arena rendered;
  for (Count index = 0; index < errors.get_size(); index++) {
    if (Algorithm::search(errors.render_message(rendered, index), diagnostic) !=
        Count(-1)) {
      return True;
    }
  }
  return False;
}

static auto rejects_link_without_publication(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (monograph || errors.is_empty()) {
    return False;
  }

  return retains_library_source(workspace, "InitializerTest"_view);
}

static Harness InitializerTests = {
  .name = "Tetrodotoxin::Library::Language::Expressions::Initializer"_view,
};

static auto is_four_zero_values(
    const Language::Model::Pack& value,
    const Language::Model::Type& type) -> Bool {
  if (!value.is_identity<Language::Expressions::Initializer>()) {
    return False;
  }

  const auto& initializer =
      static_cast<const Language::Expressions::Initializer&>(value);
  auto completed = initializer.get_completed_values();
  if (&initializer.get_type() != &type || !completed ||
      completed->get_layout().get_size() != Count(4)) {
    return False;
  }

  for (Count index = 0; index < Count(4); index++) {
    auto entry = completed->get_layout().get_abstract(index);
    if (!entry || !entry->is<Language::Constants::Unsigned>() ||
        static_cast<const Language::Constants::Unsigned&>(*entry).get_value() !=
            U64(0)) {
      return False;
    }
  }

  return True;
}

PERIMORTEM_UNIT_TEST(InitializerTests, value_defaults) {
  static constexpr View::Bytes source =
      "// Non Object default construction.\n"
      "dialect : Library;\n"
      "public scalar : U32 = new[U32];\n"
      "public bytes : Fixed[U8, 4] = "
      "new[Fixed[U8, 4]];"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const auto& scalar = static_cast<const Language::Field&>(
      monograph->get_source().resolve_concept("scalar"_view));
  const auto& bytes = static_cast<const Language::Field&>(
      monograph->get_source().resolve_concept("bytes"_view));
  auto authored_scalar = scalar.get_initializer();
  auto authored_bytes = bytes.get_initializer();
  auto scalar_type = scalar.get_type().select<Language::Model::Type>();
  auto bytes_type = bytes.get_type().select<Language::Model::Type>();
  ASSERT(authored_scalar && authored_bytes && scalar_type && bytes_type);

  Perimortem::Memory::Allocator::Arena direct_values;
  auto direct_scalar = scalar_type->create_default(direct_values);
  auto direct_bytes = bytes_type->create_default(direct_values);
  ASSERT(direct_scalar && direct_bytes);
  ASSERT(authored_scalar->is_identity<Language::Expressions::Initializer>());
  ASSERT(direct_scalar->is_identity<Language::Constants::Unsigned>());
  const auto& authored_initializer =
      static_cast<const Language::Expressions::Initializer&>(*authored_scalar);
  auto authored_value = authored_initializer.get_completed_values();
  ASSERT(
      authored_value &&
      authored_value->is_identity<Language::Constants::Unsigned>());
  const auto& authored_unsigned =
      static_cast<const Language::Constants::Unsigned&>(*authored_value);
  const auto& direct_unsigned =
      static_cast<const Language::Constants::Unsigned&>(*direct_scalar);
  EXPECT(&authored_unsigned.get_type() == &*scalar_type);
  EXPECT(&direct_unsigned.get_type() == &*scalar_type);
  EXPECT_EQ(authored_unsigned.get_value(), U64(0));
  EXPECT_EQ(direct_unsigned.get_value(), U64(0));
  EXPECT(is_four_zero_values(*authored_bytes, *bytes_type));
  EXPECT(is_four_zero_values(*direct_bytes, *bytes_type));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, object_arguments) {
  static constexpr View::Bytes source =
      "// Initializer test.\n"
      "dialect : Library;\n"
      "public Defaults : object { public cache : Bool; public state enabled : "
      "Bool = false; public state count : U64; }\n"
      "public Required : object {\n"
      "  public state first : U64;\n"
      "  private state hidden : Bool = false;\n"
      "  expose state second : Bool = false;\n"
      "}\n"
      "public empty : Defaults = new[Defaults];\n"
      "public configured : Required = "
      "new[Required](.second = true, .first = 4,);"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const auto& source_type = monograph->get_source();
  const auto& defaults = static_cast<const Language::Types::Object&>(
      source_type.resolve_concept("Defaults"_view));
  const auto& required = static_cast<const Language::Types::Object&>(
      source_type.resolve_concept("Required"_view));
  const auto& empty_field = static_cast<const Language::Field&>(
      source_type.resolve_concept("empty"_view));
  const auto& configured_field = static_cast<const Language::Field&>(
      source_type.resolve_concept("configured"_view));
  auto empty_initializer = empty_field.get_initializer();
  auto configured_initializer = configured_field.get_initializer();
  ASSERT(empty_initializer);
  ASSERT(configured_initializer);
  ASSERT(empty_initializer->is_identity<Language::Expressions::Initializer>());
  ASSERT(configured_initializer
             ->is_identity<Language::Expressions::Initializer>());
  const auto& empty = static_cast<const Language::Expressions::Initializer&>(
      *empty_initializer);
  const auto& configured =
      static_cast<const Language::Expressions::Initializer&>(
          *configured_initializer);
  EXPECT(&empty.get_type() == &defaults);
  EXPECT(&configured.get_type() == &required);

  ASSERT(empty.get_completed_values());
  ASSERT(configured.get_completed_values());
  const Layout& empty_values = empty.get_completed_values()->get_layout();
  ASSERT_EQ(empty_values.get_size(), Count(2));
  auto empty_count = empty_values.get_abstract(1);
  ASSERT(empty_count && empty_count->is<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*empty_count)
          .get_value(),
      U64(0));
  const Layout& configured_values =
      configured.get_completed_values()->get_layout();
  ASSERT_EQ(configured_values.get_size(), Count(3));
  auto configured_first = configured_values.get_abstract(0);
  auto configured_hidden = configured_values.get_abstract(1);
  auto configured_second = configured_values.get_abstract(2);
  ASSERT(
      configured_first &&
      configured_first->is<Language::Constants::Unsigned>());
  ASSERT(
      configured_hidden && configured_hidden->is<Language::Constants::False>());
  ASSERT(
      configured_second && configured_second->is<Language::Constants::True>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*configured_first)
          .get_value(),
      U64(4));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, private_arguments) {
  static constexpr View::Bytes source =
      "// Descendant Object initialization.\n"
      "dialect : Library;\n"
      "public Owner : object {\n"
      "  private state hidden : Bool = false;\n"
      "  public Builder : struct {\n"
      "    private value : Owner = new[Owner](.hidden = true);\n"
      "  }\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const auto& owner = static_cast<const Language::Types::Object&>(
      monograph->get_source().resolve_concept("Owner"_view));
  const auto& builder = static_cast<const Language::Types::Structure&>(
      owner.resolve_concept("Builder"_view));
  auto fields = builder.get_addressables();
  ASSERT(fields != fields.end());
  const auto& value = static_cast<const Language::Field&>((*fields).get());
  auto initializer = value.get_initializer();
  ASSERT(
      initializer &&
      initializer->is_identity<Language::Expressions::Initializer>());
  const auto& created =
      static_cast<const Language::Expressions::Initializer&>(*initializer);
  EXPECT(&created.get_type() == &owner);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, inferred_object) {
  static constexpr View::Bytes source =
      "// Inferred initializer test.\n"
      "dialect : Library;\n"
      "public Session : object { public state active : Bool; }\n"
      "public inferred := new[Session];"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const auto& source_type = monograph->get_source();
  const Abstract& session = source_type.resolve_concept("Session"_view);
  const auto& inferred = static_cast<const Language::Field&>(
      source_type.resolve_concept("inferred"_view));
  EXPECT(&inferred.get_type() == &session);
  auto initializer = inferred.get_initializer();
  ASSERT(
      initializer &&
      initializer->is_identity<Language::Expressions::Initializer>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, explicit_scalar_conversion) {
  static constexpr View::Bytes source =
      "// Explicit scalar construction.\n"
      "dialect : Library;\n"
      "public const narrow_signed : S8 = new[S8](300);\n"
      "public const narrow_unsigned : U8 = new[U8](300.9);\n"
      "public const rounded_real : R32 = new[R32](16777217);"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const auto& signed_field = static_cast<const Language::Field&>(
      monograph->get_source().resolve_concept("narrow_signed"_view));
  const auto& unsigned_field = static_cast<const Language::Field&>(
      monograph->get_source().resolve_concept("narrow_unsigned"_view));
  const auto& real_field = static_cast<const Language::Field&>(
      monograph->get_source().resolve_concept("rounded_real"_view));
  auto signed_value = signed_field.get_constant();
  auto unsigned_value = unsigned_field.get_constant();
  auto real_value = real_field.get_constant();
  ASSERT(signed_value && unsigned_value && real_value);
  auto selected_signed =
      signed_value->select_identity<Language::Constants::Signed>();
  auto selected_unsigned =
      unsigned_value->select_identity<Language::Constants::Unsigned>();
  auto selected_real = real_value->select_identity<Language::Constants::Real>();
  ASSERT(selected_signed && selected_unsigned && selected_real);
  EXPECT_EQ(selected_signed->get_value(), S64(127));
  EXPECT_EQ(selected_unsigned->get_value(), U64(255));
  EXPECT_EQ(selected_real->get_value(), R64(16777216.0));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, argument_rejections) {
  struct Rejection {
    View::Bytes source;
    View::Bytes diagnostic;
  };
  static constexpr Rejection rejections[] = {
    {
      "// Non Object arguments.\ndialect : Library;\npublic invalid : U32 = new[U32](.value = 1);"_view,
      "Selected scalar Type cannot construct this value."_view,
    },
    {
      "// Positional Object argument.\ndialect : Library;\npublic Session : object { public state value : U64; }\npublic invalid : Session = new[Session](5);"_view,
      "Object initializer inputs do not fit the initialization Layout."_view,
    },
    {
      "// Duplicate Object argument.\ndialect : Library;\npublic Session : object { public state value : U64; }\npublic invalid : Session = new[Session](.value = 1, .value = 2);"_view,
      "Duplicate name in one Library Pack."_view,
    },
    {
      "// Mixed Object arguments.\ndialect : Library;\npublic Session : object { public state value : U64; }\npublic invalid : Session = new[Session](1, .value = 2);"_view,
      "Positional and named entries cannot share one Library Pack."_view,
    },
    {
      "// Empty Object arguments.\ndialect : Library;\npublic Session : object { public state value : U64; }\npublic invalid : Session = new[Session]();"_view,
      "Initializer arguments cannot be empty."_view,
    },
  };

  EXPECT(
      rejects_interpretation(rejections[0].source, rejections[0].diagnostic));
  EXPECT(
      rejects_interpretation(rejections[1].source, rejections[1].diagnostic));
  EXPECT(
      rejects_interpretation(rejections[2].source, rejections[2].diagnostic));
  EXPECT(
      rejects_interpretation(rejections[3].source, rejections[3].diagnostic));
  EXPECT(
      rejects_interpretation(rejections[4].source, rejections[4].diagnostic));
}

PERIMORTEM_UNIT_TEST(InitializerTests, nested_defaults) {
  static constexpr View::Bytes source =
      "// Nested Object default test.\n"
      "dialect : Library;\n"
      "public Inner : object { private state value : U64; }\n"
      "public Outer : object {\n"
      "  private state inner : Inner; private state enabled : Bool;\n"
      "}\n"
      "public created : Outer = new[Outer];"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& inner = monograph->get_source().resolve_concept("Inner"_view);
  const auto& created = static_cast<const Language::Field&>(
      monograph->get_source().resolve_concept("created"_view));
  auto value = created.get_initializer();
  ASSERT(value && value->is_identity<Language::Expressions::Initializer>());
  const auto& initializer =
      static_cast<const Language::Expressions::Initializer&>(*value);
  ASSERT(initializer.get_completed_values());
  const Layout& values = initializer.get_completed_values()->get_layout();
  ASSERT_EQ(values.get_size(), Count(2));
  auto nested = values.get_abstract(0);
  auto enabled = values.get_abstract(1);
  ASSERT(nested && nested->is<Language::Expressions::Initializer>());
  ASSERT(enabled && enabled->is<Language::Constants::False>());
  EXPECT(
      &static_cast<const Language::Expressions::Initializer&>(*nested)
           .get_type() == &inner);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(InitializerTests, field_rejections) {
  static constexpr View::Bytes sources[] = {
    "// Wrong result Type.\ndialect : Library;\npublic Session : object { public state value : Bool; }\npublic invalid : Bool = new[Session];"_view,
    "// Unknown input.\ndialect : Library;\npublic Session : object { public state value : U64; }\npublic invalid : Session = new[Session](.missing = 1);"_view,
    "// Private input.\ndialect : Library;\npublic Session : object { private state hidden : Bool = false; }\npublic invalid : Session = new[Session](.hidden = true);"_view,
    "// Input Type mismatch.\ndialect : Library;\npublic Session : object { public state value : U64; }\npublic invalid : Session = new[Session](.value = false);"_view,
    "// Const input.\ndialect : Library;\npublic Session : object { public const value : U64 = 1; }\npublic invalid : Session = new[Session](.value = 2);"_view,
  };

  for (View::Bytes source : sources) {
    EXPECT(rejects_link_without_publication(source));
  }
}

PERIMORTEM_UNIT_TEST(InitializerTests, object_cycles) {
  static constexpr View::Bytes static_override =
      "// Static initializer ownership test.\n"
      "dialect : Library;\n"
      "public Session : object { public value : U64 = 1; }\n"
      "public invalid : Session = new[Session](.value = 2);"_view;
  EXPECT(rejects_link_without_publication(static_override));

  static constexpr View::Bytes source =
      "// Initializer cycle test.\n"
      "dialect : Library;\n"
      "public Node : object { private state next : Node = new[Node]; }\n"
      "public invalid : Node = new[Node];"_view;
  EXPECT(rejects_link_without_publication(source));

  static constexpr View::Bytes missing_default =
      "// Missing default cycle test.\n"
      "dialect : Library;\n"
      "public Node : object { private state next : Node; }\n"
      "public invalid : Node = new[Node];"_view;
  EXPECT(rejects_link_without_publication(missing_default));

  static constexpr View::Bytes optional =
      "// Optional default cycle test.\n"
      "dialect : Library;\n"
      "public Node : object { private state next : Option[Node]; }\n"
      "public valid : Node = new[Node];"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, optional);
  ASSERT(monograph);
  const auto& valid = static_cast<const Language::Field&>(
      monograph->get_source().resolve_concept("valid"_view));
  auto initializer = valid.get_initializer();
  ASSERT(
      initializer &&
      initializer->is_identity<Language::Expressions::Initializer>());
  const auto& value =
      static_cast<const Language::Expressions::Initializer&>(*initializer);
  ASSERT(value.get_completed_values());
  auto next = value.get_completed_values()->get_layout().get_abstract(0);
  ASSERT(next && next->is<Language::Constants::Option>());
  EXPECT(errors.is_empty());
}
