// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/interface.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/implementation.hpp"
#include "tetrodotoxin/library/language/types/implemented.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness InterfaceTypes = {
  .name = "Tetrodotoxin::Library::Language::Types::Interface"_view,
};

PERIMORTEM_UNIT_TEST(InterfaceTypes, materializes_implementation_surface) {
  static constexpr View::Bytes source =
      "// Interface Type test.\n"
      "dialect : Library;\n"
      "public DrawableUI : interface {\n"
      "  public state visible : Bool = true;\n"
      "  public state z_index : S64;\n"
      "}\n"
      "public Sprite : implementation DrawableUI {\n"
      "  public state texture : U64;\n"
      "}\n"
      "public Holder : object {\n"
      "  public state drawable : Implementation[DrawableUI];\n"
      "}\n"
      "public is_visible : func = [\n"
      "  .drawable : Implementation[DrawableUI],\n"
      "] -> Bool {\n"
      "  return drawable.visible;\n"
      "}"_view;

  Tetrodotoxin::Library::Dialect toolchain_library;

  auto toolchain = Validation::create_library_toolchain(toolchain_library);
  Environment::Workspace workspace(*toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "InterfaceTypes"_view, "interface.ttx"_view, source);
  ASSERT(interpreted);
  ASSERT(interpreted->is<Tetrodotoxin::Library::Language::Monograph>());
  ASSERT(errors.is_empty());

  const auto& monograph =
      static_cast<const Tetrodotoxin::Library::Language::Monograph&>(
          *interpreted);
  const Abstract& requirement = monograph.resolve_concept("DrawableUI"_view);
  const Abstract& candidate = monograph.resolve_concept("Sprite"_view);
  ASSERT(requirement.is<Tetrodotoxin::Library::Language::Types::Interface>());
  ASSERT(candidate.is<Tetrodotoxin::Library::Language::Types::Implemented>());
  EXPECT(
      static_cast<const Tetrodotoxin::Library::Language::Model::Type&>(
          requirement)
          .get_layout()
          .is_empty());
  EXPECT(candidate.satisfies(requirement));

  const auto& sprite =
      static_cast<const Tetrodotoxin::Library::Language::Types::Implemented&>(
          candidate);
  auto fields = sprite.get_addressables();
  ASSERT(fields != fields.end());
  const auto& visible =
      static_cast<const Tetrodotoxin::Library::Language::Field&>(
          (*fields).get());
  ++fields;
  ASSERT(fields != fields.end());
  const auto& z_index =
      static_cast<const Tetrodotoxin::Library::Language::Field&>(
          (*fields).get());
  ++fields;
  ASSERT(fields != fields.end());
  const auto& texture =
      static_cast<const Tetrodotoxin::Library::Language::Field&>(
          (*fields).get());
  ++fields;
  EXPECT(fields == sprite.get_addressables().end());
  EXPECT_TEXT(visible.get_name(), "visible"_view);
  EXPECT_TEXT(z_index.get_name(), "z_index"_view);
  EXPECT_TEXT(texture.get_name(), "texture"_view);
  ASSERT(visible.get_initializer());
  ASSERT_EQ(sprite.get_layout().get_size(), Count(3));

  const auto& holder =
      static_cast<const Tetrodotoxin::Library::Language::Types::Object&>(
          monograph.resolve_concept("Holder"_view));
  auto holder_fields = holder.get_addressables();
  ASSERT(holder_fields != holder_fields.end());
  const auto& drawable =
      static_cast<const Tetrodotoxin::Library::Language::Field&>(
          (*holder_fields).get());
  ASSERT(drawable.get_type()
             .is<Tetrodotoxin::Library::Language::Types::Implementation>());
  const auto& erased = static_cast<
      const Tetrodotoxin::Library::Language::Types::Implementation&>(
      drawable.get_type());
  EXPECT(&erased.get_requirement().resolve() == &requirement);

  Allocator::Arena values;
  auto concrete = sprite.create_default(values);
  ASSERT(concrete);
  EXPECT(erased.accepts(*concrete));
}

PERIMORTEM_UNIT_TEST(InterfaceTypes, rejects_duplicate_and_wrong_requirement) {
  static constexpr View::Bytes duplicate =
      "// Duplicate Interface state.\n"
      "dialect : Library;\n"
      "public DrawableUI : interface { public state visible : Bool; }\n"
      "public Sprite : implementation DrawableUI {\n"
      "  public state visible : Bool;\n"
      "}"_view;
  static constexpr View::Bytes wrong_requirement =
      "// Wrong implementation requirement.\n"
      "dialect : Library;\n"
      "public DrawableUI : struct {}\n"
      "public Sprite : implementation DrawableUI {}"_view;

  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    duplicate,
    wrong_requirement,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    Tetrodotoxin::Library::Dialect toolchain_library;
    auto toolchain = Validation::create_library_toolchain(toolchain_library);
    Environment::Workspace workspace(*toolchain);
    Errors errors;
    auto interpreted = workspace.interpret_source(
        errors, "RejectedInterface"_view, "interface.ttx"_view, sources[index]);
    EXPECT_NOT(interpreted);
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(InterfaceTypes, archive_round_trip) {
  static constexpr View::Bytes source =
      "// Interface Archive test.\n"
      "dialect : Library;\n"
      "public DrawableUI : interface {\n"
      "  public state visible : Bool = true;\n"
      "  public state z_index : S64;\n"
      "}\n"
      "public Sprite : implementation DrawableUI {\n"
      "  public state texture : U64;\n"
      "}"_view;

  Tetrodotoxin::Library::Dialect toolchain_library;

  auto toolchain = Validation::create_library_toolchain(toolchain_library);
  Environment::Workspace workspace(*toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "InterfaceArchive"_view, "interface.ttx"_view, source);
  ASSERT(
      interpreted &&
      interpreted->is<Tetrodotoxin::Library::Language::Monograph>());
  ASSERT(errors.is_empty());
  auto& monograph =
      static_cast<Tetrodotoxin::Library::Language::Monograph&>(*interpreted);
  auto& dialect = get_library_dialect(*toolchain);
  auto encoded = dialect.encode(monograph);
  ASSERT(encoded);

  Allocator::Arena restored_domain;
  auto decoded = dialect.decode(restored_domain, *encoded, workspace);
  auto restored =
      decoded ? decoded->select<Tetrodotoxin::Library::Language::Monograph>()
              : Option<Tetrodotoxin::Library::Language::Monograph&>();
  ASSERT(restored);
  ASSERT(restored->link_restored());
  ASSERT(restored->finalize_restored());
  ASSERT(restored->is<Tetrodotoxin::Library::Language::Monograph>());
  const auto& restored_library =
      static_cast<const Tetrodotoxin::Library::Language::Monograph&>(*restored);
  const Abstract& requirement =
      restored_library.resolve_concept("DrawableUI"_view);
  const Abstract& candidate = restored_library.resolve_concept("Sprite"_view);
  ASSERT(requirement.is<Tetrodotoxin::Library::Language::Types::Interface>());
  ASSERT(candidate.is<Tetrodotoxin::Library::Language::Types::Implemented>());
  EXPECT(candidate.satisfies(requirement));
  const auto& sprite =
      static_cast<const Tetrodotoxin::Library::Language::Types::Implemented&>(
          candidate);
  ASSERT_EQ(sprite.get_layout().get_size(), Count(3));
}
