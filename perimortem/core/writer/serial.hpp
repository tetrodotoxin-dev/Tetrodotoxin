// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::Core::Writer {

// Reads self describing values from a serial bytes following the Perimortem
// Serial format.
//
// Values are encoded as 1 type byte along with there minimum byte form and are
// always returned as 64 bit values regardless of how they were encoded.
//
// Blobs are encoded as a Value + a string of raw bytes. The Value's type byte
// contains an additional flag encoding it's a blob.
//
// Negative Values are encoded as their two's complement for easier compression
// with their type byte marked with the negate flag.
//
// On overflow the writer enters an invalid state and subsequent writes are safe
// but treated as undefined behavior.
class Serial {
 public:
  constexpr Serial(Access::Bytes source) : source(source) {};
  constexpr Serial(const Serial& rhs) : source(rhs.source) {};

  auto set_pointer(Count location) -> void;

  auto operator<<(const U64 value) -> Serial&;
  auto operator<<(const View::Bytes blob) -> Serial&;

  constexpr auto get_size() const -> Count { return source.get_size(); }
  constexpr auto get_location() const -> Count { return cursor; }
  constexpr auto is_valid() const -> Bool { return valid_state; }
  constexpr operator View::Bytes() const {
    return View::Bytes(source.get_data(), cursor);
  }

 private:
  Access::Bytes source;
  Count cursor = 0;
  Bool valid_state = True;
};

}  // namespace Perimortem::Core::Writer
