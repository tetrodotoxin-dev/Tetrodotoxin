// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "tetrodotoxin/source/diagnostic.hpp"
#include "tetrodotoxin/source/lexical/cursor.h"
#include "tetrodotoxin/source/lexical/span.hpp"

namespace Tetrodotoxin::Source::Lexical {

// The C contract lends indexed token observations and diagnostics. This facade
// adds local traversal and caches the current Token because providers keep each
// index stable. Copies inherit the current observation but advance
// independently. Changing the index invalidates only that copy's cache, so
// lookahead and nested dialects need no new binding or provider-owned traversal
// state.
class Cursor {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_SOURCE_LEXICAL_CURSOR_ID_HIGH,
    TETRODOTOXIN_SOURCE_LEXICAL_CURSOR_ID_LOW};
  using Api = tetrodotoxin_source_cursor;
  explicit constexpr Cursor(Api api) : api(api) {}
  constexpr auto get_abi() const -> Api { return api; }
  constexpr auto get_index() const -> U64 { return api.index; }

  // A foreign consumer updates its C record directly. Adopting that position
  // goes through this method so the next observation cannot reuse an old token.
  auto set_index(U64 index) -> void {
    api.index = index;
    current_token = {};
  }

  auto get_token(S64 relative = 0) const -> Token {
    if (relative) {
      return api.operations->get_token(api.source, api.index + relative);
    }

    if (!current_token) {
      current_token = api.operations->get_token(api.source, api.index);
    }

    return *current_token;
  }
  auto current() const -> Token { return get_token(); }
  auto peek(S64 relative) const -> Token { return get_token(relative); }
  auto consume() -> Token {
    const auto token = current();
    if (token) {
      set_index(api.index + 1);
    }

    return token;
  }
  auto get_text(Token token) const -> Perimortem::Core::View::Bytes {
    return api.operations->get_text(api.source, token);
  }
  auto get_text() const -> Perimortem::Core::View::Bytes {
    return get_text(current());
  }
  auto get_anchor(Span span, Perimortem::Core::Option<Token> focus = {}) const
      -> Tetrodotoxin::Source::Anchor {
    auto anchor = Tetrodotoxin::Source::Anchor(
        Ttx::Concept::Abstract(ttx_none()), Tetrodotoxin::Source::Range());
    api.operations->get_anchor(
        api.source, span, focus ? &*focus : nullptr, &anchor);
    return anchor;
  }
  auto get_anchor(Token token) const -> Tetrodotoxin::Source::Anchor {
    return get_anchor(Span(token), token);
  }
  auto get_error_count() const -> Count {
    return api.operations->get_error_count(api.source);
  }
  auto get_error(Count index) const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Diagnostic> {
    Tetrodotoxin::Source::Diagnostic error;
    if (!api.operations->get_error(api.source, index, &error)) {
      return {};
    }
    return error;
  }
  auto report(
      Perimortem::Core::Option<Tetrodotoxin::Source::Anchor> anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    api.operations->report(
        api.source, anchor ? &*anchor : nullptr, message, hint);
  }
  auto create_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    report({}, message, hint);
  }
  auto create_token_error(
      Token token,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    report(get_anchor(token), message, hint);
  }
  auto create_token_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    create_token_error(current(), message, hint);
  }
  auto create_expression_error(
      Span span,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    report(get_anchor(span, span.get_start()), message, hint);
  }
  auto matches(Code::Type type) const -> Bool {
    return current().get_code() == type;
  }
  auto get_code() const -> Code { return current().get_code(); }
  auto is_one_of(Perimortem::Core::View::Vector<Code::Type> types) const
      -> Bool {
    return current().get_code().is_one_of(types);
  }
  auto require(Code::Type type, Perimortem::Core::View::Bytes message = {})
      -> Token;
  auto recover_to_statement(
      Perimortem::Core::View::Vector<Code::Type> terminals = {
        {Code::Type::Terminal, Code::Type::EndStatement,
         Code::Type::ScopeEnd}}) -> void;
  auto recover_to_scoped_statement() -> void;

 private:
  Api api;
  mutable Perimortem::Core::Option<Token> current_token;
};

}  // namespace Tetrodotoxin::Source::Lexical

TTX_DATA_RECORD(
    tetrodotoxin_source_cursor_ops,
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor_ops, get_token),
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor_ops, get_text),
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor_ops, get_anchor),
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor_ops, get_error_count),
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor_ops, report),
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor_ops, get_error));

TTX_DATA_RECORD(
    tetrodotoxin_source_cursor,
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor, source),
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor, operations),
    TTX_DATA_MEMBER(tetrodotoxin_source_cursor, index));
