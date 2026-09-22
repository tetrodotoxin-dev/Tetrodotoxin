// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/source/interface.hpp"

namespace Tetrodotoxin::Shader::Language {

// Contract negotiates whether one real Shader Program can occupy the role of a
// Render Structure. Callable Layouts provide data flow evidence while Render
// Attributes and Shader binding relationships restore the policy that Layout
// intentionally omits. Neither side is copied into the other language.
class Contract : public Tetrodotoxin::Source::Interface {
 public:
  auto negotiate(
      const Tetrodotoxin::Source::Abstract& requirement,
      const Tetrodotoxin::Source::Abstract& candidate) const -> Relation override;

  auto validate(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& requirement,
      const Tetrodotoxin::Source::Abstract& candidate) const -> Bool;

  // Source free validation has no Cursor for authored presentation. It applies
  // the same relation and publishes the exact lost contract fact through the
  // process diagnostic boundary owned by Archive restoration.
  auto validate_restored(
      const Tetrodotoxin::Source::Abstract& requirement,
      const Tetrodotoxin::Source::Abstract& candidate) const -> Bool;
};

}  // namespace Tetrodotoxin::Shader::Language
