// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/cursor.h"
#include "tetrodotoxin/dialects/source/span.hpp"
#include "tetrodotoxin/dialects/source/stream.hpp"

namespace Tetrodotoxin::Dialects::Source {

// Cursor caches the admitted reader while its caller retains the Stream.
// Range boundaries act as Terminal without hiding the next enclosing token.
// Recovery code chooses its own stop set and whether to consume that boundary.
// A foreign call borrows the caller's state directly, so nested operations have
// one position throughout the call rather than reconciling copies afterward.
class Cursor {
 public:
  explicit Cursor(Stream stream)
      : tokens(stream.get_tokens()),
        input(stream.get_input()),
        owned{
          stream.get_abi(), 0, 0,
          tokens.get_size() ? tokens.get_size() - 1 : 0},
        state(owned) {}
  explicit Cursor(tetrodotoxin_source_cursor& value)
      : tokens(Stream(value.stream).get_tokens()),
        input(Stream(value.stream).get_input()),
        state(value) {}
  Cursor(const Cursor&) = delete;
  auto operator=(const Cursor&) -> Cursor& = delete;
  auto is_valid() const -> Bool {
    return tokens.is_valid() && state.begin <= state.index &&
           state.index <= state.end && state.end < tokens.get_size() &&
           (input.text.data || !input.text.size) &&
           (input.path.data || !input.path.size);
  }
  auto get_abi() -> tetrodotoxin_source_cursor& { return state; }
  auto get_stream() const -> Stream { return Stream(state.stream); }
  auto current() const -> Token {
    if (state.index < state.begin || state.index > state.end ||
        state.end >= tokens.get_size()) {
      return Token();
    }
    auto token = tokens[state.index];
    return state.index == state.end
               ? Token(
                     token.get_offset(), token.get_line(), token.get_column(),
                     0, Code::Type::Terminal)
               : token;
  }
  auto peek(S64 offset) const -> Token {
    if (!is_valid()) {
      return Token();
    }
    U64 index = state.index;
    if (offset < 0) {
      const U64 distance = U64(-(offset + 1)) + 1;
      if (distance > state.index - state.begin) {
        return Token();
      }
      index -= distance;
    } else {
      if (U64(offset) > state.end - state.index) {
        return Token();
      }
      index += U64(offset);
    }
    auto token = tokens[index];
    return index == state.end ? Token(
                                    token.get_offset(), token.get_line(),
                                    token.get_column(), 0, Code::Type::Terminal)
                              : token;
  }
  auto consume() -> Token {
    auto token = current();
    if (token) {
      ++state.index;
    }
    return token;
  }
  auto consume_if(Code::Type type) -> Token {
    return matches(type) ? consume() : Token();
  }
  auto skip_until(Perimortem::Core::View::Vector<Code::Type> boundaries)
      -> void {
    while (current() && !is_one_of(boundaries)) {
      consume();
    }
  }
  auto matches(Code::Type type) const -> Bool {
    return current().get_code() == type;
  }
  auto is_one_of(Perimortem::Core::View::Vector<Code::Type> types) const
      -> Bool {
    return current().get_code().is_one_of(types);
  }
  auto get_code() const -> Code { return current().get_code(); }
  auto get_text() const -> Perimortem::Core::View::Bytes {
    return current().caculate_text(get_source_text());
  }
  auto caculate_text(Span span) const -> Perimortem::Core::View::Bytes {
    return span.caculate_text(get_source_text());
  }
  auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return {input.text.data, input.text.size};
  }
  auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return {input.path.data, input.path.size};
  }
  auto get_tokens() const -> Tokens { return tokens; }

 private:
  Tokens tokens;
  tetrodotoxin_source_snapshot input;
  tetrodotoxin_source_cursor owned{};
  tetrodotoxin_source_cursor& state;
};
}  // namespace Tetrodotoxin::Dialects::Source

TTX_DATA_RECORD(
    tetrodotoxin_source_cursor,
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor, stream),
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor, begin),
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor, index),
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor, end));
