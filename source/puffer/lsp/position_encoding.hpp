// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

namespace Puffer::Lsp {

// TTX already measures source locations as UTF 8 byte offsets, which is also
// the most natural LSP representation for Puffer. Some editor clients count
// UTF 16 code units instead, so this small boundary lets both sides agree
// without teaching TTX about an editor protocol.
class PositionEncoding {
 public:
  enum class Kind { Utf8, Utf16 };

  class Position {
   public:
    constexpr Position(Count line, Count character)
        : line(line), character(character) {}

    constexpr auto get_line() const -> Count { return line; }

    constexpr auto get_character() const -> Count { return character; }

    constexpr auto is_before(const Position& other) const -> Bool {
      return line < other.line ||
             (line == other.line && character < other.character);
    }

   private:
    Count line;
    Count character;
  };

  constexpr PositionEncoding() : kind(Kind::Utf16) {}
  constexpr explicit PositionEncoding(Kind kind) : kind(kind) {}

  constexpr auto get_kind() const -> Kind { return kind; }
  auto get_name() const -> Perimortem::Core::View::Bytes;

  auto locate(Perimortem::Core::View::Bytes source, Count byte_offset) const
      -> Perimortem::Core::Option<Position>;
  auto find_offset(
      Perimortem::Core::View::Bytes source,
      const Position& position) const -> Perimortem::Core::Option<Count>;

 private:
  Kind kind;
};

}  // namespace Puffer::Lsp
