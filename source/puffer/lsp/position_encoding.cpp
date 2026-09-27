// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/position_encoding.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Puffer::Lsp;

struct Utf8Codepoint {
  Count bytes;
  Count utf_16_units;
};

// UTF 8 and UTF 16 differ here only in the width assigned to one decoded
// character. Decoding once also keeps either mode from accepting a position in
// the middle of a source character.
static auto decode(View::Bytes source, Count offset, Count limit)
    -> Utf8Codepoint {
  U8 lead = source[offset];
  Count bytes = 1;
  Count units = 1;
  if (lead >= 0xC2 && lead <= 0xDF) {
    bytes = 2;
  } else if (lead >= 0xE0 && lead <= 0xEF) {
    bytes = 3;
  } else if (lead >= 0xF0 && lead <= 0xF4) {
    bytes = 4;
    units = 2;
  }

  if (offset + bytes > limit) {
    return {1, 1};
  }
  for (Count index = 1; index < bytes; index++) {
    if ((source[offset + index] & 0xC0) != 0x80) {
      return {1, 1};
    }
  }
  return {bytes, units};
}

auto PositionEncoding::get_name() const -> View::Bytes {
  return kind == Kind::Utf8 ? "utf-8"_view : "utf-16"_view;
}

auto PositionEncoding::locate(View::Bytes source, Count byte_offset) const
    -> Option<Position> {
  BAIL_IF(byte_offset > source.get_size());

  Count line = 0;
  Count character = 0;
  Count offset = 0;
  while (offset < byte_offset) {
    if (source[offset] == '\n') {
      line++;
      character = 0;
      offset++;
      continue;
    }

    Utf8Codepoint codepoint = decode(source, offset, source.get_size());
    BAIL_IF(offset + codepoint.bytes > byte_offset);
    offset += codepoint.bytes;
    character += kind == Kind::Utf8 ? codepoint.bytes : codepoint.utf_16_units;
  }
  return Position(line, character);
}

auto PositionEncoding::find_offset(View::Bytes source, const Position& position)
    const -> Option<Count> {
  Count offset = 0;
  Count line = 0;
  while (line < position.get_line() && offset < source.get_size()) {
    if (source[offset++] == '\n') {
      line++;
    }
  }
  BAIL_IF(line != position.get_line());

  Count character = 0;
  while (offset < source.get_size() && source[offset] != '\n' &&
         character < position.get_character()) {
    Utf8Codepoint codepoint = decode(source, offset, source.get_size());
    Count units = kind == Kind::Utf8 ? codepoint.bytes : codepoint.utf_16_units;
    BAIL_IF(character + units > position.get_character());
    offset += codepoint.bytes;
    character += units;
  }
  return character == position.get_character() ? Option<Count>(offset)
                                               : Option<Count>();
}
