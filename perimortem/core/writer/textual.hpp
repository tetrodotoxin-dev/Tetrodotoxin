// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"

namespace Perimortem::Core::Writer {

// Reads human readable values from a text byte buffer. Reads are greedy so
// numeric values must be whitespace seperated to be read appropriately.
//
// Numeric and boolean reads automatically skip leading whitespace except for
// read_byte() which never skips whitespace and always consume exactly one byte
// from the current position.
//
// On overflow the writer enters an invalid state and subsequent writes are safe
// but treated as undefined behavior.
class Textual {
 public:
  Textual(Core::Access::Bytes source) : source(source) {};
  Textual(const Textual& rhs) : source(rhs.source) {};

  // Sets the location of the read/write cursor.
  // If the index is out of range then the cursor is put to the end of the
  // buffer.
  auto set_pointer(Count location) -> void;

  auto operator<<(char character) -> Textual&;
  auto operator<<(const Bool flag) -> Textual&;
  auto operator<<(const U8 byte) -> Textual&;
  auto operator<<(const U16 value) -> Textual&;
  auto operator<<(const U32 value) -> Textual&;
  auto operator<<(const U64 value) -> Textual&;
  auto operator<<(const S8 value) -> Textual&;
  auto operator<<(const S16 value) -> Textual&;
  auto operator<<(const S32 value) -> Textual&;
  auto operator<<(const S64 value) -> Textual&;
  auto operator<<(const R32 value) -> Textual&;
  auto operator<<(const R64 value) -> Textual&;
  auto operator<<(const View::Bytes raw) -> Textual&;

  // Don't let C++ promote or do anything funky with C Strings.
  // They aren't part of the ABI so we explicitly delete them.
  auto operator<<(const char* raw) -> Textual& = delete;

  constexpr auto get_size() const -> Count { return source.get_size(); }
  constexpr auto get_location() const -> Count { return cursor; }
  constexpr auto is_valid() const -> Bool { return valid_state; }
  constexpr operator View::Bytes() const {
    return View::Bytes(source.get_data(), cursor);
  }

 private:
  auto write_real(R64 real, R64 precision) -> void;
  Access::Bytes source;
  Count cursor = 0;
  Bool valid_state = true;
};

}  // namespace Perimortem::Core::Writer
