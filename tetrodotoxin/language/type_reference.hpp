// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Language {

// TypeReference keeps one authored Type route without Generic arguments until
// the surrounding semantic island is ready to answer it. It retains only the
// source spelling and Anchor, so each consuming Dialect remains responsible for
// its Type system.
class TypeReference {
 public:
  static constexpr auto create(
      Perimortem::Core::View::Bytes route,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> TypeReference {
    return TypeReference(route, anchor);
  }

  auto resolve(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& context) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Type&>;

  // A concrete declaration owner may select the first route segment through
  // its private lexical policy, then return to ordinary public Type context
  // queries for every explicit `::` suffix.
  auto resolve_selected(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& selected_root) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Type&>;

  // Archive restoration follows the same semantic route after every owner has
  // reconstructed its identities. It has no authored Cursor to report through,
  // so absence lets the persistent Dialect reject the complete transaction.
  auto resolve_restored(const Tetrodotoxin::Source::Abstract& context) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Type&>;

  auto resolve_restored_selected(const Tetrodotoxin::Source::Abstract& selected_root)
      const -> Perimortem::Core::Option<const Tetrodotoxin::Source::Type&>;

  auto get_root() const -> Perimortem::Core::View::Bytes;

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

 private:
  constexpr TypeReference(
      Perimortem::Core::View::Bytes route,
      Tetrodotoxin::Source::Lexical::Anchor anchor)
      : route(route), anchor(anchor) {}

  Perimortem::Core::View::Bytes route;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Language
