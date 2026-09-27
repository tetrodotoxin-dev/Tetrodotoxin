// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/linker/import.hpp"

namespace Tetrodotoxin::Linker {

// Provider is one build selected native symbol definition. Selection compares
// target ABI kind and spelling together so a same named state or incompatible
// target cannot accidentally satisfy an Import.
class Provider {
 public:
  enum class Error : U8 {
    Unknown = U8(-1),
    Missing = 0,
    Ambiguous,
  };

  constexpr Provider(
      Perimortem::Core::View::Bytes identity,
      Perimortem::Core::View::Bytes target,
      Import::Kind kind,
      Perimortem::Core::View::Bytes abi,
      Perimortem::Core::View::Bytes symbol)
      : identity(identity),
        target(target),
        kind(kind),
        abi(abi),
        symbol(symbol) {}

  static auto select(
      Perimortem::Core::View::Vector<Provider> providers,
      Perimortem::Core::View::Bytes target,
      const Import& imported) -> Perimortem::Utility::Result<Import, Error>;

  constexpr auto get_identity() const -> Perimortem::Core::View::Bytes {
    return identity;
  }

  constexpr auto get_target() const -> Perimortem::Core::View::Bytes {
    return target;
  }

  constexpr auto get_kind() const -> Import::Kind { return kind; }

  constexpr auto get_abi() const -> Perimortem::Core::View::Bytes {
    return abi;
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

 private:
  Perimortem::Core::View::Bytes identity;
  Perimortem::Core::View::Bytes target;
  Import::Kind kind;
  Perimortem::Core::View::Bytes abi;
  Perimortem::Core::View::Bytes symbol;
};

}  // namespace Tetrodotoxin::Linker
