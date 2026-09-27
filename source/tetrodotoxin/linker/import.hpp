// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Linker {

// Import is one unresolved native symbol required by an object artifact. The
// compiler supplies ABI and symbol facts while Package compilation attaches the
// selected logical provider for its target. Filesystem locations remain build
// policy and never enter this record.
class Import {
 public:
  enum class Kind : U8 {
    Function,
    ReadOnlyState,
    WritableState,
  };

  constexpr Import(
      Kind kind,
      Perimortem::Core::View::Bytes abi,
      Perimortem::Core::View::Bytes symbol,
      Perimortem::Core::View::Bytes provider = {})
      : kind(kind), abi(abi), symbol(symbol), provider(provider) {}

  constexpr auto get_kind() const -> Kind { return kind; }

  constexpr auto get_abi() const -> Perimortem::Core::View::Bytes {
    return abi;
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

  constexpr auto get_provider() const -> Perimortem::Core::View::Bytes {
    return provider;
  }

  constexpr auto select_provider(Perimortem::Core::View::Bytes selected) const
      -> Import {
    return Import(kind, abi, symbol, selected);
  }

  constexpr auto operator==(const Import& rhs) const -> Bool {
    return kind == rhs.kind && abi == rhs.abi && symbol == rhs.symbol &&
           provider == rhs.provider;
  }

 private:
  Kind kind;
  Perimortem::Core::View::Bytes abi;
  Perimortem::Core::View::Bytes symbol;
  Perimortem::Core::View::Bytes provider;
};

}  // namespace Tetrodotoxin::Linker
