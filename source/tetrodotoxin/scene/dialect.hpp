// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/library/dialect.hpp"

namespace Tetrodotoxin::Scene {

// Scene owns its outer source policy while borrowing the exact installed
// Library Dialect that constructs the one semantic child beneath it.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);

  explicit Dialect(Tetrodotoxin::Library::Dialect& library)
      : library(library) {}

  TTX_NAME("Scene"_view);

  auto interpret(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation,
      const Tetrodotoxin::Source::Lexical::Anchor& source_anchor,
      Tetrodotoxin::Source::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

  auto encode(const Tetrodotoxin::Source::Abstract& monograph) const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes> override;

  auto decode(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      Tetrodotoxin::Source::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Abstract&> override;

 private:
  Tetrodotoxin::Library::Dialect& library;
};

}  // namespace Tetrodotoxin::Scene
