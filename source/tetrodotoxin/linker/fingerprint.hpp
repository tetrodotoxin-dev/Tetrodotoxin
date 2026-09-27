// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

namespace Tetrodotoxin::Linker {

// Fingerprint identifies one exact target ABI description. It detects stale or
// mismatched build products and is not an integrity or security digest.
class Fingerprint {
 public:
  constexpr Fingerprint(U64 value = 0) : value(value) {}

  static auto create(Perimortem::Core::View::Bytes description) -> Fingerprint;

  static auto parse(Perimortem::Core::View::Bytes text)
      -> Perimortem::Core::Option<Fingerprint>;

  auto render(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::View::Bytes;

  constexpr auto get_value() const -> U64 { return value; }

  constexpr auto operator==(const Fingerprint& rhs) const -> Bool {
    return value == rhs.value;
  }

 private:
  U64 value;
};

}  // namespace Tetrodotoxin::Linker
