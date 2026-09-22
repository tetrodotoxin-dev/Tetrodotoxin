// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Build {

// Build is the bootstrap language. Its host supplies arguments at construction
// so tool options do not become semantic names in the source graph. The views
// and their byte storage must remain alive for this Dialect's lifetime.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);

  Dialect(
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> arguments)
      : arguments(arguments) {}

  TTX_NAME("Build"_view);

  auto get_arguments() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return arguments;
  }

  auto interpret(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation,
      const Tetrodotoxin::Source::Lexical::Anchor& source_anchor,
      Tetrodotoxin::Source::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> arguments;
};

}  // namespace Tetrodotoxin::Build
