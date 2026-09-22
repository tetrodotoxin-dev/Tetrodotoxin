// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::App::Language {

// Route keeps one authored path to an application participant. Unlike a Type
// reference, the destination may be a Scene Monograph, a Library source, or
// another contextual identity. Each segment still follows ordinary resolve
// queries, so App never copies Package names into its own symbol table.
class Route {
 public:
  static constexpr auto create_authored(
      Perimortem::Core::View::Bytes spelling,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Route {
    return Route(spelling, anchor);
  }

  auto resolve(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& context) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&>;

  constexpr auto get_spelling() const -> Perimortem::Core::View::Bytes {
    return spelling;
  }

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

 private:
  constexpr Route(
      Perimortem::Core::View::Bytes spelling,
      Tetrodotoxin::Source::Lexical::Anchor anchor)
      : spelling(spelling), anchor(anchor) {}

  Perimortem::Core::View::Bytes spelling;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::App::Language
