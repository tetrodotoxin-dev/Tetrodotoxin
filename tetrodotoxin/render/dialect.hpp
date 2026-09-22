// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Render {

// Dialect establishes the Render owned root before its declaration families
// arrive. This gives Packages and editors one stable language identity while
// later slices add contracts to the same Monograph.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);

  Dialect() = default;

  TTX_NAME("Pipeline"_view);

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
};

}  // namespace Tetrodotoxin::Render
