// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/source/anchor.hpp"
#include "tetrodotoxin/source/lexical/span.hpp"

namespace Tetrodotoxin::Source::Lexical {

// Stream is the native lexer's immutable storage. Its Entry table owns the
// relationship between compact Tokens and full source ranges. Cursor exposes
// that relationship through operations, so foreign providers can use a
// different table or generate Tokens without supplying this native
// representation.
class Stream {
 public:
  // Entry belongs to the native implementation, which uses its table index as
  // the Token locator. Source ranges need not fit in those locator bits and
  // line numbers are derived only when rendering.
  struct Entry {
    Token token;
    Tetrodotoxin::Source::Range extent;
    constexpr Entry() : token(), extent() {}
    constexpr Entry(Token token, Tetrodotoxin::Source::Range extent)
        : token(token), extent(extent) {}
  };

  constexpr Stream(
      Perimortem::Core::View::Bytes text,
      Perimortem::Core::View::Bytes path,
      Ttx::Concept::Abstract source,
      Perimortem::Core::View::Vector<Entry> entries = {})
      : text(text), path(path), source(source), entries(entries) {}

  constexpr auto get_size() const -> Count { return entries.get_size(); }
  constexpr auto operator[](Count index) const -> Token {
    if (index >= entries.get_size()) {
      return entries.get_size() ? entries[entries.get_size() - 1].token
                                : Token();
    }

    return entries[index].token;
  }
  constexpr auto get_source() const -> Ttx::Concept::Abstract { return source; }
  constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return text;
  }
  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return path;
  }

  constexpr auto get_extent(Token token) const -> Tetrodotoxin::Source::Range {
    auto extent = entries[token.get_locator()].extent;
    return token.get_code() == Code::Type::Terminal
               ? Tetrodotoxin::Source::Range(extent.get_offset(), 0)
               : extent;
  }
  constexpr auto get_text(Token token) const -> Perimortem::Core::View::Bytes {
    const auto extent = get_extent(token);
    return text.slice(extent.get_offset(), extent.get_size());
  }
  constexpr auto get_anchor(
      Span span,
      Perimortem::Core::Option<Token> focus = {}) const
      -> Tetrodotoxin::Source::Anchor {
    const auto first = get_extent(span.get_start());
    const auto last = get_extent(span.get_end());
    const auto start = first.get_offset() < last.get_offset()
                           ? first.get_offset()
                           : last.get_offset();
    const auto first_end = first.get_offset() + first.get_size();
    const auto last_end = last.get_offset() + last.get_size();
    const auto end = first_end > last_end ? first_end : last_end;
    Perimortem::Core::Option<Tetrodotoxin::Source::Range> selected;
    if (focus) {
      selected = get_extent(*focus);
    }

    return Tetrodotoxin::Source::Anchor(
        source, Tetrodotoxin::Source::Range(start, end - start), selected);
  }

 protected:
  constexpr auto set_entries(Perimortem::Core::View::Vector<Entry> value)
      -> void {
    entries = value;
  }

 private:
  Perimortem::Core::View::Bytes text;
  Perimortem::Core::View::Bytes path;
  Ttx::Concept::Abstract source;
  Perimortem::Core::View::Vector<Entry> entries;
};

}  // namespace Tetrodotoxin::Source::Lexical
