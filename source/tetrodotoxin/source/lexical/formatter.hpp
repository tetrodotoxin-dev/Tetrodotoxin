// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/source/lexical/tokenizer.hpp"

namespace Tetrodotoxin::Source::Lexical {

// Formatter projects one complete lexical graph into canonical TTX source.
// Unknown and incomplete Tokens remain ordinary inputs, so formatting never
// depends on semantic completion and never drops malformed authored content.
class Formatter {
 public:
  constexpr Formatter(const Tokenizer& tokenizer) : tokenizer(tokenizer) {}

  auto format() const -> Perimortem::Memory::Dynamic::Bytes;

 private:
  const Tokenizer& tokenizer;
};

}  // namespace Tetrodotoxin::Source::Lexical
