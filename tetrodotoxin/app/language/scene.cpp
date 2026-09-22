// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/language/scene.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto App::Language::Scene::create_authored(
    Allocator::Arena& arena,
    const Tetrodotoxin::Source::Documentation& documentation,
    Route initial,
    View::Vector<Reference<Transition>> transitions,
    Anchor anchor) -> Scene& {
  return arena.construct_from<Scene>([&]() -> Scene {
    return Scene(documentation, initial, transitions, anchor);
  });
}

auto App::Language::Scene::link(Cursor& cursor, const Abstract& context)
    -> Bool {
  auto selected = initial.resolve(cursor, context);
  auto scene =
      selected ? selected->select<Tetrodotoxin::Scene::Language::Monograph>()
               : Option<const Tetrodotoxin::Scene::Language::Monograph&>();
  if (!scene) {
    cursor.create_expression_error(
        initial.get_anchor(), "App initial route must select a Scene."_view);
    return False;
  }
  initial_scene =
      Reference<const Tetrodotoxin::Scene::Language::Monograph>(*scene);

  Bool valid = True;
  for (const Reference<Transition>& transition : transitions) {
    valid &= transition.get().link(cursor, context);
  }
  return valid;
}

auto App::Language::Scene::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return Abstract::resolve_concept(route);
}
