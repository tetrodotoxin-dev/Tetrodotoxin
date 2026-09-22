// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Library {

// Dialect implements the installed Library source protocol. Every semantic
// identity belongs to the Monograph created for that source.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);

  Dialect() = default;

  TTX_NAME("Library"_view);

  auto bind_interface(Perimortem::System::Uuid requested) const
      -> Perimortem::Utility::Result<
          Ttx::Semantic::Negotiation::Binding,
          Ttx::Semantic::Negotiation::Binding::Failure> override;

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

}  // namespace Tetrodotoxin::Library
