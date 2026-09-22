// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/dialect.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/render/language/monograph.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "tetrodotoxin/render/language/structure.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness RenderDialect = {
  .name = "Tetrodotoxin::Render::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(RenderDialect, retained_root) {
  static constexpr View::Bytes source = "//\ndialect : Pipeline;"_view;
  Render::Dialect dialect;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(dialect));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Format"_view, "format.ttx"_view, source);

  ASSERT(interpreted && interpreted->is<Render::Language::Monograph>());
  const auto& monograph =
      static_cast<const Render::Language::Monograph&>(*interpreted);
  ASSERT(monograph.get_layer(dialect));
  EXPECT(&*monograph.get_layer(dialect) == &monograph);
  EXPECT(&workspace.resolve_concept("Format"_view) == &monograph);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(RenderDialect, stage_contract) {
  static constexpr View::Bytes source =
      "//\n"
      "dialect : Pipeline;\n"
      "public fragment : stage [] -> [];"_view;
  Render::Dialect installed_pipeline;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(installed_pipeline));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Format"_view, "progressive-render.ttx"_view, source);

  ASSERT(interpreted && interpreted->is<Render::Language::Monograph>());
  const Abstract& selected = interpreted->resolve_concept("static"_view)
                                 .resolve_concept("fragment"_view)
                                 .resolve();
  ASSERT(selected.is<Render::Language::Stage>());
  const auto& stage = static_cast<const Render::Language::Stage&>(selected);
  EXPECT(stage.get_parameters().is_empty());
  EXPECT(stage.get_results().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(RenderDialect, stage_name_shape) {
  static constexpr View::Bytes source =
      "//\n"
      "dialect : Pipeline;\n"
      "public Fragment : stage [] -> [];"_view;
  Render::Dialect installed_pipeline;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(installed_pipeline));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Format"_view, "type-stage.ttx"_view, source);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(RenderDialect, structured_contract) {
  static constexpr View::Bytes values_source =
      "// Shared values.\n"
      "dialect : Library;"_view;
  static constexpr View::Bytes source =
      "// Pipeline interface.\n"
      "dialect : Pipeline;\n"
      "public Simple : struct {\n"
      "  @capability(\"fragment\")\n"
      "  public fragment : stage [\n"
      "    @location(0).color : Values::R64,\n"
      "  ] -> [\n"
      "    @location(0).color : Values::R64,\n"
      "  ];\n"
      "  @set(0) @slot(1) @read\n"
      "  public texture : resource read Values::U64;\n"
      "}"_view;
  Library::Dialect installed_library;
  Render::Dialect installed_pipeline;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(installed_library));
  ASSERT(toolchain.install(installed_pipeline));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  ASSERT(workspace.interpret_source(
      errors, "Values"_view, "values.ttx"_view, values_source));
  auto interpreted = workspace.interpret_source(
      errors, "Formats"_view, "formats.ttx"_view, source);

  ASSERT(interpreted);
  const Abstract& selected = interpreted->resolve_concept("Simple"_view);
  ASSERT(selected.is<Render::Language::Structure>());
  const auto& structure =
      static_cast<const Render::Language::Structure&>(selected);
  EXPECT(structure.resolve_concept("static"_view)
             .resolve_concept("fragment"_view)
             .resolve()
             .is<Render::Language::Stage>());
  EXPECT(structure.resolve_concept("static"_view)
             .resolve_concept("texture"_view)
             .resolve()
             .is<Render::Language::Binding>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(RenderDialect, rejects_incomplete_resource_binding) {
  static constexpr View::Bytes values_source =
      "// Shared values.\n"
      "dialect : Library;"_view;
  static constexpr View::Bytes source =
      "// Unknown Pipeline interface.\n"
      "dialect : Pipeline;\n"
      "@set(0)\n"
      "public texture : resource read Values::U64;"_view;
  Library::Dialect installed_library;
  Render::Dialect installed_pipeline;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(installed_library));
  ASSERT(toolchain.install(installed_pipeline));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  ASSERT(workspace.interpret_source(
      errors, "Values"_view, "values.ttx"_view, values_source));
  auto interpreted = workspace.interpret_source(
      errors, "Unknown"_view, "invalid-render.ttx"_view, source);

  EXPECT_NOT(interpreted);
  EXPECT_EQ(errors.get_size(), Count(1));
}
