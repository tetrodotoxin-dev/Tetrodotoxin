// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/app/language/route.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"
#include "tetrodotoxin/scene/language/signal.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::App::Language {

// Transition connects one exact Scene Signal to the stack operation App applies
// after presentation. It retains routes only until linking can select the real
// Scene and Signal identities. Runtime stack and event state remain outside the
// Monograph.
class Transition : public Tetrodotoxin::Source::Abstract {
 public:
  enum class Action : U8 {
    Replace,
    Push,
    Pop,
    Exit,
  };

  TTX_CONTRACT(Transition, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Documentation& documentation,
      Route source,
      Perimortem::Core::View::Bytes signal_name,
      Tetrodotoxin::Source::Lexical::Anchor signal_anchor,
      Action action,
      Perimortem::Core::Option<Route> destination,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Transition&;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor, const Tetrodotoxin::Source::Abstract& context)
      -> Bool;

  constexpr auto get_source_route() const -> const Route& { return source; }
  constexpr auto get_signal_name() const -> Perimortem::Core::View::Bytes {
    return signal_name;
  }
  constexpr auto get_action() const -> Action { return action; }
  constexpr auto get_destination_route() const
      -> const Perimortem::Core::Option<Route>& {
    return destination;
  }
  constexpr auto get_source_scene() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Scene::Language::Monograph&> {
    return source_scene.visit(
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
  constexpr auto get_signal() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Scene::Language::Signal&> {
    return signal.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Scene::Language::Signal&> { return {}; },
        [](const Tetrodotoxin::Source::Reference<
            const Tetrodotoxin::Scene::Language::Signal>& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Scene::Language::Signal&> {
          return selected.get();
        });
  }
  constexpr auto get_destination_scene() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Scene::Language::Monograph&> {
    return destination_scene.visit(
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

  TTX_NAME(signal_name);
  TTX_DOCUMENTATION(documentation);

  auto resolve_concept(Perimortem::Core::View::Bytes) const
      -> const Tetrodotoxin::Source::Abstract& override;

 private:
  constexpr Transition(
      const Tetrodotoxin::Source::Documentation& documentation,
      Route source,
      Perimortem::Core::View::Bytes signal_name,
      Tetrodotoxin::Source::Lexical::Anchor signal_anchor,
      Action action,
      Perimortem::Core::Option<Route> destination,
      Tetrodotoxin::Source::Lexical::Anchor anchor)
      : documentation(documentation),
        source(source),
        signal_name(signal_name),
        signal_anchor(signal_anchor),
        action(action),
        destination(destination),
        anchor(anchor) {}

  const Tetrodotoxin::Source::Documentation& documentation;
  Route source;
  Perimortem::Core::View::Bytes signal_name;
  Tetrodotoxin::Source::Lexical::Anchor signal_anchor;
  Action action;
  Perimortem::Core::Option<Route> destination;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Scene::Language::Monograph>>
      source_scene;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Scene::Language::Signal>>
      signal;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Scene::Language::Monograph>>
      destination_scene;
};

}  // namespace Tetrodotoxin::App::Language
