// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/source/lexical/stream.hpp"

namespace Tetrodotoxin::Source::Lexical {

// Tokenizer partitions one borrowed byte stream into ordered TTX Tokens. Each
// Token selects a provider owned range so text remains owned once.
//
// Any byte stream has a token representation. Unrecognized spans receive the
// Unknown Code rather than requiring semantic feedback during tokenization.
class Tokenizer : public Stream {
 public:
  Tokenizer(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Bytes source_path,
      Ttx::Concept::Abstract source = Ttx::Concept::Abstract(ttx_none()))
      : Stream(source_text, source_path, source) {
    parse(arena);
  }

  // The tokenizer is empty if it has 0 or 1 (Terminal) tokens.
  constexpr auto is_empty() const -> Bool { return get_size() <= 1; }

 private:
  auto parse(Perimortem::Memory::Allocator::Arena& arena) -> void;
};

}  // namespace Tetrodotoxin::Source::Lexical
