// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Render::Language {

// Attributes owns the Render meaning of otherwise uninterpreted authored keys.
// Validation proves that each key appears on a declaration where its meaning is
// coherent. Satisfaction compares a concrete implementation with the required
// Render facts while both sides keep their original semantic identities.
class Attributes {
 public:
  enum class Placement : U8 {
    Value,
    Push,
    Resource,
    Stage,
    StageEntry,
    Structure,
  };

  Attributes() = delete;

  static auto validate(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
          attributes,
      Placement placement) -> Bool;

  // Archive reconstruction has no source Cursor. This pure query applies the
  // same key, value, repetition, and pairing policy so malformed payload facts
  // cannot enter a restored graph.
  static auto accepts(
      Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
          attributes,
      Placement placement) -> Bool;

  // Interface negotiation can test the same policy without manufacturing a
  // diagnostic context. The owner that presents a failed relationship reports
  // that complete relationship at its own boundary.
  static auto satisfies(
      Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
          supplied,
      Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
          required) -> Bool;
};

}  // namespace Tetrodotoxin::Render::Language
