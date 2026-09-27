// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Scene::Language {

// Signal is one event published by a Scene. It keeps the authored name and an
// optional Library Type edge because event policy belongs to Scene while the
// transported value keeps its real Library meaning. A Signal owns no queue or
// subscriber state. Those appear only after an application creates a live
// Scene instance.
class Signal : public Tetrodotoxin::Source::Abstract {
 public:
  TTX_CONTRACT(Signal, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Documentation& documentation,
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Source::Lexical::Token name_token,
      Perimortem::Core::Option<Tetrodotoxin::Library::Language::TypeReference>
          payload,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Signal&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Documentation& documentation,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::Option<Tetrodotoxin::Library::Language::TypeReference>
          payload) -> Signal&;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor, const Tetrodotoxin::Source::Abstract& context)
      -> Bool;

  auto link_restored(const Tetrodotoxin::Source::Abstract& context) -> Bool;

  constexpr auto get_payload_reference() const
      -> const Perimortem::Core::Option<
          Tetrodotoxin::Library::Language::TypeReference>& {
    return payload;
  }

  constexpr auto get_payload_type() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Library::Language::Model::Type&> {
    return payload_type;
  }

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

  constexpr auto is_linked() const -> Bool { return linked; }

  constexpr auto resolve() const -> const Tetrodotoxin::Source::Abstract& override {
    return *this;
  }

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

 private:
  constexpr Signal(
      const Tetrodotoxin::Source::Documentation& documentation,
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Source::Lexical::Token name_token,
      Perimortem::Core::Option<Tetrodotoxin::Library::Language::TypeReference>
          payload,
      Tetrodotoxin::Source::Lexical::Anchor anchor)
      : documentation(documentation),
        name(name),
        name_token(name_token),
        payload(payload),
        anchor(anchor) {}

  const Tetrodotoxin::Source::Documentation& documentation;
  Perimortem::Core::View::Bytes name;
  Tetrodotoxin::Source::Lexical::Token name_token;
  Perimortem::Core::Option<Tetrodotoxin::Library::Language::TypeReference>
      payload;
  Perimortem::Core::Option<const Tetrodotoxin::Library::Language::Model::Type&>
      payload_type;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Scene::Language
