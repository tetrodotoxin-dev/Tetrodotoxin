// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/type.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness TypeAccessTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Type"_view,
};

static auto links_library_source(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Environment::Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "TypeAccessTest"_view, "type-access.ttx"_view, source);
  return interpreted && errors.is_empty();
}

static auto rejects_library_link(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Environment::Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "TypeAccessTest"_view, "type-access.ttx"_view, source);
  return !interpreted && !errors.is_empty();
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, qualified_authority) {
  static constexpr View::Bytes accepted =
      "// Qualified private Type access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  private Hidden : struct { private state value : Bool; }\n"
      "  public Inner : struct { private value : Hidden; }\n"
      "}"_view;
  static constexpr Static::Vector<View::Bytes, 2> rejected = {{
    "// Self qualified private Type access.\n"
    "dialect : Library;\n"
    "public Outer : struct {\n"
    "  private Hidden : struct { private state value : Bool; }\n"
    "  public Inner : struct { private value : Outer::Hidden; }\n"
    "}"_view,
    "// External private Type access.\n"
    "dialect : Library;\n"
    "public Outer : struct {\n"
    "  private Hidden : struct { private state value : Bool; }\n"
    "}\n"
    "public Borrowed : alias = Outer;\n"
    "private invalid : Borrowed::Hidden;"_view,
  }};

  EXPECT(links_library_source(accepted));
  for (Count index = 0; index < rejected.get_size(); index++) {
    EXPECT(rejects_library_link(rejected[index]));
  }
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, local_shadowing) {
  static constexpr View::Bytes source =
      "// Local Type root shadowing.\n"
      "dialect : Library;\n"
      "public Host : struct {\n"
      "  public Bool : struct {\n"
      "    public Nested : struct { private state value : U8; }\n"
      "  }\n"
      "  public value : Bool::Nested;\n"
      "}"_view;

  EXPECT(links_library_source(source));
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, authored_descriptor) {
  static constexpr View::Bytes source =
      "// Descriptor is an ordinary authored Type name.\n"
      "dialect : Library;\n"
      "public Descriptor : struct {\n"
      "  public Nested : struct { private state value : U8; }\n"
      "}\n"
      "private value : Descriptor::Nested;"_view;

  EXPECT(links_library_source(source));
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, no_pack_flow) {
  static constexpr View::Bytes sources[] = {
    "// Type result as a Field value.\ndialect : Library; public Packet : struct { private state value : Bool; } private invalid := Packet;"_view,
    "// Type result as a Local value.\ndialect : Library; public Packet : struct {} private invalid : func = [] -> [] { state value := Packet; return; }"_view,
    "// Type result inside grouped flow.\ndialect : Library; public Packet : struct {} private invalid : Bool = (Packet, false);"_view,
    "// Type result as an initializer argument.\ndialect : Library; public Packet : struct {} public Target : object { public state value : Bool; } private invalid := new[Target](.value = Packet);"_view,
    "// Type result as a Call argument.\ndialect : Library; public Packet : struct {} public Calls : struct { public use : func = [.value : Bool] -> Bool { return value; } } private invalid := Calls -> use(Packet);"_view,
    "// Type result as a return value.\ndialect : Library; public Packet : struct {} private invalid : func = [] -> Bool { return Packet; }"_view,
    "// Type result as a branch condition.\ndialect : Library; public Packet : struct {} private invalid : func = [] -> [] { if Packet { return; } return; }"_view,
    "// Type result as an assignment source.\ndialect : Library; public Packet : struct {} private invalid : func = [] -> [] { state value : Bool = false; value = Packet; return; }"_view,
    "// Type result as a swizzle receiver.\ndialect : Library; public Packet : struct { public state value : Bool; } private invalid := Packet.[value];"_view,
    "// Type result as a match input.\ndialect : Library; public Packet : struct {} private invalid : func = [] -> [] { match Packet { case Packet {} } return; }"_view,
  };

  for (View::Bytes source : sources) {
    EXPECT(rejects_library_link(source));
  }
}

PERIMORTEM_UNIT_TEST(TypeAccessTests, expression_access) {
  static constexpr View::Bytes accepted =
      "// Type-valued expression access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  public Nested : struct {\n"
      "    public create : func = [] -> Bool { return true; }\n"
      "  }\n"
      "}\n"
      "private selected := Outer::Nested -> create();"_view;
  static constexpr View::Bytes value_qualification =
      "// Value cannot provide Type qualification.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  private state value : Bool;\n"
      "  public Nested : struct {}\n"
      "}\n"
      "private value : Outer;\n"
      "private invalid := value::Nested;"_view;
  static constexpr View::Bytes type_address =
      "// A Structure Type identity does not prove static storage.\n"
      "dialect : Library;\n"
      "public Packet : struct { expose state value : Bool = false; }\n"
      "private invalid := Packet.value;"_view;
  static constexpr View::Bytes object_type_address =
      "// An Object Type identity does not prove static storage.\n"
      "dialect : Library;\n"
      "public Packet : object { public state value : Bool = false; }\n"
      "private invalid := Packet.value;"_view;
  static constexpr View::Bytes const_type_address =
      "// A Structure Type selects its compile-time const Field.\n"
      "dialect : Library;\n"
      "public Packet : struct { public const value : Bool = false; }\n"
      "private selected := Packet.value;"_view;
  static constexpr View::Bytes const_object_type_address =
      "// An Object Type selects its compile-time const Field.\n"
      "dialect : Library;\n"
      "public Packet : object { public const value : Bool = false; }\n"
      "private selected := Packet.value;"_view;
  static constexpr View::Bytes source_address =
      "// Source Fields have unambiguous static storage.\n"
      "dialect : Library;\n"
      "public value : Bool = false;\n"
      "private selected := source.value;"_view;

  EXPECT(links_library_source(accepted));
  EXPECT(rejects_library_link(value_qualification));
  EXPECT(rejects_library_link(type_address));
  EXPECT(rejects_library_link(object_type_address));
  EXPECT(links_library_source(const_type_address));
  EXPECT(links_library_source(const_object_type_address));
  EXPECT(links_library_source(source_address));
}
