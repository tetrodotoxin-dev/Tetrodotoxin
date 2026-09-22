// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"
#include "tetrodotoxin/terminal/graphics/compiler.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness SceneDialect = {
  .name = "Tetrodotoxin::Scene::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(SceneDialect, exact_layers) {
  static constexpr View::Bytes source =
      "//\n"
      "dialect : Scene;\n"
      "private state active : Bool = false;\n"
      "Scene prepare[self] -> [] {}\n"
      "Scene update[self, .delta_time : R64] -> [] {}\n"
      "Scene release[self] -> [] {}"_view;
  Library::Dialect library;
  Scene::Dialect dialect(library);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(dialect));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Empty"_view, "empty-scene.ttx"_view, source);

  ASSERT(interpreted && interpreted->is<Scene::Language::Monograph>());
  const auto& immutable =
      static_cast<const Scene::Language::Monograph&>(*interpreted);
  const auto& child = immutable.get_library();
  Allocator::Arena other_domain;
  Library::Dialect other_library;
  Scene::Dialect other_scene(library);
  ASSERT(immutable.get_layer(dialect));
  ASSERT(immutable.get_layer(library));
  EXPECT(&*immutable.get_layer(dialect) == &immutable);
  EXPECT(&*immutable.get_layer(library) == &child);
  EXPECT_NOT(immutable.get_layer(other_scene));
  EXPECT_NOT(immutable.get_layer(other_library));
  EXPECT(&child.resolve_concept("Empty"_view) == &immutable);
  ASSERT_EQ(
      child.get_documentation().line_count(),
      immutable.get_documentation().line_count());
  EXPECT_TEXT(
      child.get_documentation().get_line(0),
      immutable.get_documentation().get_line(0));
  EXPECT(&workspace.resolve_concept("Empty"_view) == &immutable);
  EXPECT(&workspace.resolve_concept("Library"_view) == &Unknown::get_unknown());
  EXPECT(child.get_source().is_linked());
  EXPECT(child.get_source().is_finalized());
  EXPECT(&workspace.resolve_concept("Empty"_view) == &immutable);
  EXPECT(&workspace.resolve_concept("Library"_view) == &Unknown::get_unknown());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SceneDialect, child_rejection) {
  static constexpr View::Bytes scene_source =
      "//\n"
      "dialect : Scene;\n"
      "using Missing;\n"
      "Scene prepare[self] -> [] {}\n"
      "Scene update[self, .delta_time : R64] -> [] {}\n"
      "Scene release[self] -> [] {}"_view;
  Library::Dialect library;
  Scene::Dialect installed_scene(library);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(installed_scene));
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "Broken"_view, "broken-scene.ttx"_view, scene_source);

  EXPECT_NOT(interpreted);
  EXPECT(workspace.resolve_concept("Broken"_view)
             .is<Scene::Language::Monograph>());
  EXPECT_EQ(errors.get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(SceneDialect, delayed_declarations) {
  static constexpr View::Bytes source =
      "//\n"
      "dialect : Scene;\n"
      "signal later;"_view;
  Library::Dialect library;
  Scene::Dialect installed_scene(library);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(installed_scene));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Rejected"_view, "scene-declaration.ttx"_view, source);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
  auto retained = workspace.get_monograph("scene-declaration.ttx"_view);
  ASSERT(retained && retained->is<Scene::Language::Monograph>());
  const auto& scene = static_cast<const Scene::Language::Monograph&>(*retained);
  EXPECT(scene.find_signal("later"_view));
  EXPECT(&workspace.resolve_concept("Rejected"_view) == &*retained);
}

PERIMORTEM_UNIT_TEST(SceneDialect, library_declarations) {
  static constexpr View::Bytes source =
      "// Scene with ordinary CPU meaning.\n"
      "dialect : Scene;\n"
      "public Item : struct {\n"
      "  public state value : U64 = 7;\n"
      "}\n"
      "private state active : Bool = false;\n"
      "public read : func = [self] -> U64 : return 7;\n"
      "Scene prepare[self] -> [] {}\n"
      "Scene update[self, .delta_time : R64] -> [] {}\n"
      "Scene release[self] -> [] {}"_view;
  Library::Dialect library;
  Scene::Dialect installed_scene(library);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(installed_scene));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Owned"_view, "owned-scene.ttx"_view, source);

  ASSERT(interpreted && interpreted->is<Scene::Language::Monograph>());
  const auto& scene =
      static_cast<const Scene::Language::Monograph&>(*interpreted);
  EXPECT(scene.resolve_concept("Item"_view)
             .resolve()
             .is<Library::Language::Types::Structure>());
  EXPECT(scene.resolve_concept("instance"_view)
             .resolve_concept("read"_view)
             .resolve()
             .is<Library::Language::Function>());
  EXPECT(scene.get_library().get_source().is_linked());
  EXPECT(scene.get_library().get_source().is_finalized());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SceneDialect, expands_fixed_hosted_objects) {
  static constexpr View::Bytes source =
      "// Scene with a homogeneous hosted collection.\n"
      "dialect : Scene;\n"
      "public Drawable : interface {\n"
      "  public state visible : Bool = true;\n"
      "}\n"
      "public Icon : implementation Drawable {}\n"
      "private state icons : Fixed[Icon, 2];\n"
      "Scene prepare[self] -> [] {}\n"
      "Scene update[self, .delta_time : R64] -> [] {}\n"
      "Scene release[self] -> [] {}"_view;
  Library::Dialect library;
  Scene::Dialect installed_scene(library);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(installed_scene));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  auto interpreted = workspace.interpret_source(
      errors, "Collection"_view, "collection-scene.ttx"_view, source);
  ASSERT(interpreted && interpreted->is<Scene::Language::Monograph>());
  const auto& scene =
      static_cast<const Scene::Language::Monograph&>(*interpreted);
  auto requirement = scene.resolve_concept("Drawable"_view)
                         .resolve()
                         .select<Tetrodotoxin::Source::Type>();
  auto icon =
      scene.resolve_concept("Icon"_view).resolve().select<Tetrodotoxin::Source::Type>();
  ASSERT(requirement && icon);

  Static::Vector<Reference<const Tetrodotoxin::Source::Type>, 1> configured = {{*icon}};
  Allocator::Arena arena;
  auto products = Terminal::Graphics::Compiler().compile(
      arena, scene, *requirement, configured);
  ASSERT(products);
  auto hosted = products->get_hosted();
  ASSERT_EQ(hosted.get_size(), Count(2));
  const auto& first = hosted.get_data()[0];
  const auto& second = hosted.get_data()[1];
  ASSERT(first.get_element_index());
  ASSERT(second.get_element_index());
  EXPECT_EQ(*first.get_element_index(), Count(0));
  EXPECT_EQ(*second.get_element_index(), Count(1));
  EXPECT(&first.get_field() == &second.get_field());
  EXPECT_EQ(first.get_type_index(), Count(0));
  EXPECT_EQ(second.get_type_index(), Count(0));
  EXPECT(errors.is_empty());
}
