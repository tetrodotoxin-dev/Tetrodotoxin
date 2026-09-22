// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/interfaces/structure.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness StructureInterface = {
  .name = "Tetrodotoxin::Library::Language::Interfaces::Structure"_view,
};

PERIMORTEM_UNIT_TEST(StructureInterface, negotiates_real_library_types) {
  static constexpr View::Bytes source =
      "// Structure Interface contract.\n"
      "dialect : Library;\n"
      "public Transform : struct { public state x : R64; }\n"
      "public Host : struct {\n"
      "  public state transform : Transform;\n"
      "  public state visible : Bool;\n"
      "  public state z_index : S64;\n"
      "}\n"
      "public Sprite : object {\n"
      "  public state transform : Transform;\n"
      "  public state visible : Bool;\n"
      "  public state z_index : S64;\n"
      "  public state texture : U64;\n"
      "}\n"
      "public WrongType : object {\n"
      "  public state transform : Transform;\n"
      "  public state visible : Bool;\n"
      "  public state z_index : U64;\n"
      "}\n"
      "public HiddenState : object {\n"
      "  public state transform : Transform;\n"
      "  private state visible : Bool;\n"
      "  public state z_index : S64;\n"
      "}\n"
      "public InlineValue : struct {\n"
      "  public state transform : Transform;\n"
      "  public state visible : Bool;\n"
      "  public state z_index : S64;\n"
      "}"_view;

  Library::Dialect installed_library;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(installed_library));
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto monograph = workspace.interpret_source(
      errors, "Interfaces"_view, "structure-interface.ttx"_view, source);
  ASSERT(monograph);
  ASSERT(errors.is_empty());

  const Abstract& requirement = monograph->resolve_concept("Host"_view);
  const Abstract& sprite = monograph->resolve_concept("Sprite"_view);
  const Abstract& wrong_type = monograph->resolve_concept("WrongType"_view);
  const Abstract& hidden_state = monograph->resolve_concept("HiddenState"_view);
  const Abstract& inline_value = monograph->resolve_concept("InlineValue"_view);
  Library::Language::Interfaces::Structure hosting;

  EXPECT(
      hosting.negotiate(requirement, requirement) ==
      Tetrodotoxin::Source::Interface::Relation::Rejected);
  EXPECT(
      hosting.negotiate(requirement, sprite) ==
      Tetrodotoxin::Source::Interface::Relation::Satisfied);
  EXPECT(
      hosting.negotiate(requirement, wrong_type) ==
      Tetrodotoxin::Source::Interface::Relation::Rejected);
  EXPECT(
      hosting.negotiate(requirement, hidden_state) ==
      Tetrodotoxin::Source::Interface::Relation::Rejected);
  EXPECT(
      hosting.negotiate(requirement, inline_value) ==
      Tetrodotoxin::Source::Interface::Relation::Rejected);
}
