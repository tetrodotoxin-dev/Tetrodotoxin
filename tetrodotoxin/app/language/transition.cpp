// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/language/transition.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto App::Language::Transition::create_authored(
    Allocator::Arena& arena,
    const Tetrodotoxin::Source::Documentation& documentation,
    Route source,
    View::Bytes signal_name,
    Anchor signal_anchor,
    Action action,
    Option<Route> destination,
    Anchor anchor) -> Transition& {
  return arena.construct_from<Transition>([&]() -> Transition {
    return Transition(
        documentation, source, signal_name, signal_anchor, action, destination,
        anchor);
  });
}

static auto select_scene(const Option<const Abstract&>& selected)
    -> Option<const Scene::Language::Monograph&> {
  return selected ? selected->select<Scene::Language::Monograph>()
                  : Option<const Scene::Language::Monograph&>();
}

auto App::Language::Transition::link(Cursor& cursor, const Abstract& context)
    -> Bool {
  auto selected_source = select_scene(source.resolve(cursor, context));
  if (!selected_source) {
    cursor.create_expression_error(
        source.get_anchor(), "App transition source must select a Scene."_view);
    return False;
  }
  auto selected_signal = selected_source->find_signal(signal_name);
  if (!selected_signal) {
    cursor.create_expression_error(
        signal_anchor,
        "App transition names a Signal that its source Scene does not publish."_view);
    return False;
  }

  Option<const Scene::Language::Monograph&> selected_destination;
  if (destination) {
    selected_destination = select_scene(destination->resolve(cursor, context));
    if (!selected_destination) {
      cursor.create_expression_error(
          destination->get_anchor(),
          "App transition destination must select a Scene."_view);
      return False;
    }
  }
  BAIL_IF(
      (action == Action::Replace || action == Action::Push) !=
      bool(selected_destination));

  source_scene = Reference<const Scene::Language::Monograph>(*selected_source);
  signal = Reference<const Scene::Language::Signal>(*selected_signal);
  if (selected_destination) {
    destination_scene =
        Reference<const Scene::Language::Monograph>(*selected_destination);
  }
  cursor.get_associations().create(signal_anchor, *selected_signal);
  return True;
}

auto App::Language::Transition::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return Abstract::resolve_concept(route);
}
