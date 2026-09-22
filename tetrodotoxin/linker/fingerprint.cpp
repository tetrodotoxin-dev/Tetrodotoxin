// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/linker/fingerprint.hpp"

#include "perimortem/core/hash.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;

auto Tetrodotoxin::Linker::Fingerprint::create(Core::View::Bytes description)
    -> Fingerprint {
  return Fingerprint(Core::Hash(description).get_value());
}

auto Tetrodotoxin::Linker::Fingerprint::parse(Core::View::Bytes text)
    -> Core::Option<Fingerprint> {
  if (text.get_size() != 16) {
    return {};
  }

  U64 value = 0;
  for (Count index = 0; index < text.get_size(); index++) {
    U8 byte = text[index];
    U8 digit = 0;
    if (byte >= '0' && byte <= '9') {
      digit = byte - '0';
    } else if (byte >= 'a' && byte <= 'f') {
      digit = byte - 'a' + 10;
    } else {
      return {};
    }
    value = (value << 4) | digit;
  }
  return Fingerprint(value);
}

auto Tetrodotoxin::Linker::Fingerprint::render(
    Memory::Allocator::Arena& arena) const -> Core::View::Bytes {
  constexpr Core::View::Bytes digits = "0123456789abcdef"_view;
  Memory::Managed::Bytes output(arena);
  for (Count index = 0; index < 16; index++) {
    Count shift = (15 - index) * 4;
    output.append(digits[(value >> shift) & 15]);
  }
  return output.get_view();
}
