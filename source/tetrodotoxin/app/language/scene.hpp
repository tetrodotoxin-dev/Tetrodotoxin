// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/app/language/route.hpp"
#include "tetrodotoxin/app/language/transition.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::App::Language {

// Scene owns the application policy for one initial Scene and an ordered set of
// Signal transitions. It does not own the live stack. Linking replaces every
// route with relationships to the real Scene and Signal graph identities.
class Scene : public Tetrodotoxin::Source::Abstract {
 public:
  TTX_CONTRACT(Scene, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Documentation& documentation,
      Route initial,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<Transition>>
          transitions,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Scene&;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor, const Tetrodotoxin::Source::Abstract& context)
      -> Bool;

  constexpr auto get_initial_route() const -> const Route& { return initial; }
  constexpr auto get_initial_scene() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Scene::Language::Monograph&> {
    return initial_scene.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Scene::Language::Monograph&> {
          return {};
        },
        [](const Tetrodotoxin::Source::Reference<
            const Tetrodotoxin::Scene::Language::Monograph>& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Scene::Language::Monograph&> {
          return selected.get();
        });
  }
  constexpr auto get_transitions() const { return transitions; }

  TTX_NAME("Scene"_view);
  TTX_DOCUMENTATION(documentation);

  auto resolve_concept(Perimortem::Core::View::Bytes) const
      -> const Tetrodotoxin::Source::Abstract& override;

 private:
  constexpr Scene(
      const Tetrodotoxin::Source::Documentation& documentation,
      Route initial,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<Transition>>
          transitions,
      Tetrodotoxin::Source::Lexical::Anchor anchor)
      : documentation(documentation),
        initial(initial),
        transitions(transitions),
        anchor(anchor) {}

  const Tetrodotoxin::Source::Documentation& documentation;
  Route initial;
  Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<Transition>>
      transitions;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Scene::Language::Monograph>>
      initial_scene;
};

}  // namespace Tetrodotoxin::App::Language
