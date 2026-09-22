// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/library/dialect.hpp"

namespace Tetrodotoxin::Package {

// Dialect parses one restricted Library export surface. Workspace owns the
// imported source graph, completion, and lifetime.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);

  explicit Dialect(Tetrodotoxin::Library::Dialect& library)
      : library(library) {}

  TTX_NAME("Package"_view);

  auto interpret(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation,
      const Tetrodotoxin::Source::Lexical::Anchor& source_anchor,
      Tetrodotoxin::Source::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

  // Package encoding needs ownership of its source closure. It remains
  // unsupported here until that responsibility moves out of Workspace.

  constexpr auto get_library() const -> Tetrodotoxin::Library::Dialect& {
    return library;
  }

 private:
  Tetrodotoxin::Library::Dialect& library;
};

}  // namespace Tetrodotoxin::Package
