// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/dialects/source/stream.hpp"

namespace Tetrodotoxin::Dialects::Source {

// Formatter projects one complete lexical graph into canonical TTX source.
// Unknown and incomplete Tokens remain ordinary inputs, so formatting never
// depends on semantic completion and never drops malformed authored content.
class Formatter {
 public:
  constexpr Formatter(Stream stream) : stream(stream) {}

  // Refuse an unsupported vocabulary or encoding before classifying its bytes.
  auto format() const -> Perimortem::Utility::Result<
      Perimortem::Memory::Dynamic::Bytes,
      Ttx::Semantic::Negotiation::Binding::Failure>;

 private:
  Stream stream;
};

}  // namespace Tetrodotoxin::Dialects::Source
