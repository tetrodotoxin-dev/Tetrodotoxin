// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/capabilities/import.hpp"

namespace Tetrodotoxin::Dialects::Source {

// Source interprets a described text input into a lexical Stream. The importer
// is immutable and owns no source cache. Each accepted invocation supplies one
// independent result. Unknown tokens preserve malformed source evidence, while
// values outside the compact token profile refuse the import before
// publication.
class Dialect {
 public:
  static auto importer() -> Ttx::Concept::Capabilities::Import;
};

}  // namespace Tetrodotoxin::Dialects::Source
