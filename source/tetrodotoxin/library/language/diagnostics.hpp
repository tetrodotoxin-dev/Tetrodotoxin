// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/source/layout.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

namespace Tetrodotoxin::Library::Language {

// Diagnostics renders the semantic facts already owned by Abstract, Pack, and
// Layout. It retains no parallel names or Type model. Each report queries the
// exact graph identities available at the failure boundary.
class Diagnostics {
 public:
  static auto write_type(
      Tetrodotoxin::Source::Lexical::Errors::Report& report,
      const Tetrodotoxin::Source::Abstract& abstract) -> void;

  static auto write_layout(
      Tetrodotoxin::Source::Lexical::Errors::Report& report,
      const Tetrodotoxin::Source::Layout& layout) -> void;

  static auto write_pack(
      Tetrodotoxin::Source::Lexical::Errors::Report& report,
      const Model::Pack& pack) -> void;
};

}  // namespace Tetrodotoxin::Library::Language
