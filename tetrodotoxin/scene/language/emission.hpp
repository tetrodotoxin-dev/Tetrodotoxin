// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/flow/scope.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/scene/language/signal.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Scene::Language {

// Emission is the Scene meaning retained by one Library Statement. Library
// still owns source order, lexical scope, and nested control flow, while this
// identity connects an optional value Pack to one exact Signal. The runtime
// Terminal later turns that relationship into an event without placing a live
// queue in either language graph.
class Emission : public Tetrodotoxin::Source::Abstract {
 public:
  TTX_CONTRACT(Emission, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Source::Abstract& scene,
      Perimortem::Core::View::Bytes signal_name,
      Tetrodotoxin::Source::Lexical::Token signal_token,
      Perimortem::Core::Option<Tetrodotoxin::Library::Language::Model::Pack&>
          payload,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Emission&;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Library::Language::Flow::Scope& scope) -> Bool;

  auto validate(Tetrodotoxin::Source::Lexical::Cursor& cursor) const -> Bool;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void;

  constexpr auto get_signal() const -> Perimortem::Core::Option<const Signal&> {
    return signal;
  }

  constexpr auto get_payload() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Library::Language::Model::Pack&> {
    return payload.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Library::Language::Model::Pack&> {
          return {};
        },
        [](const Tetrodotoxin::Library::Language::Model::Pack& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Library::Language::Model::Pack&> {
          return selected;
        });
  }

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  TTX_NAME(signal_name);
  TTX_EMPTY_DOCUMENTATION();

 private:
  constexpr Emission(
      Tetrodotoxin::Source::Abstract& scene,
      Perimortem::Core::View::Bytes signal_name,
      Tetrodotoxin::Source::Lexical::Token signal_token,
      Perimortem::Core::Option<Tetrodotoxin::Library::Language::Model::Pack&>
          payload,
      Tetrodotoxin::Source::Lexical::Anchor anchor)
      : scene(scene),
        signal_name(signal_name),
        signal_token(signal_token),
        payload(payload),
        anchor(anchor) {}

  Tetrodotoxin::Source::Abstract& scene;
  Perimortem::Core::View::Bytes signal_name;
  Tetrodotoxin::Source::Lexical::Token signal_token;
  Perimortem::Core::Option<Tetrodotoxin::Library::Language::Model::Pack&>
      payload;
  Perimortem::Core::Option<const Signal&> signal;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Scene::Language
