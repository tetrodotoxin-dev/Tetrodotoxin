// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/callable.hpp"

namespace Tetrodotoxin::Terminal::Abi {

// Export retains one callable and its completed native symbol. The symbol bytes
// belong to either authored source storage or the compilation Arena.
class Export {
 public:
  constexpr Export(
      const Tetrodotoxin::Source::Callable& callable,
      Perimortem::Core::View::Bytes symbol)
      : callable(callable), symbol(symbol) {}

  constexpr auto get_callable() const -> const Tetrodotoxin::Source::Callable& {
    return callable.get();
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

 private:
  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Callable> callable;
  Perimortem::Core::View::Bytes symbol;
};

}  // namespace Tetrodotoxin::Terminal::Abi
