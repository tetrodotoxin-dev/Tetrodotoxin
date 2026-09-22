// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Terminal::Abi {

// A declaration reaches a native Package interface when each authored host on
// its route is also visible. Keeping this check beside Publication lets every
// native producer share the boundary derived from the real semantic graph.
auto is_publicly_reachable(const Tetrodotoxin::Language::Definition& definition)
    -> Bool;

// Publication connects one semantic identity reachable from Package to the
// native symbol defined by this member object. Package later supplies the
// member and host route framing while this record preserves exact identity.
class Publication {
 public:
  constexpr Publication(
      const Tetrodotoxin::Source::Abstract& semantic,
      Perimortem::Core::View::Bytes symbol)
      : semantic(semantic), symbol(symbol) {}

  constexpr auto get_semantic() const -> const Tetrodotoxin::Source::Abstract& {
    return semantic.get();
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

 private:
  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> semantic;
  Perimortem::Core::View::Bytes symbol;
};

}  // namespace Tetrodotoxin::Terminal::Abi
