// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

namespace Tetrodotoxin::Language {

// The consumer constructs Report because only it has the authored source name,
// body, and range. A concrete Error contributes its retained owner facts to
// that Report without moving source or publication state into this cross
// Dialect contract.
class Error : public Tetrodotoxin::Source::Abstract {
 public:
  TTX_CONTRACT(Error, Tetrodotoxin::Source::Abstract);

  TTX_NAME("Error"_view);

  TTX_EMPTY_DOCUMENTATION();

  virtual auto describe(Tetrodotoxin::Source::Lexical::Errors::Report& report) const -> void = 0;
};

}  // namespace Tetrodotoxin::Language
