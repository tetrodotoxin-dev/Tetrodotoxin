// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/lexical/cursor.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;

auto Cursor::require(Code::Type type, View::Bytes message) -> Token {
  if (matches(type)) {
    return consume();
  }

  // The grammar's message supplies intent. Cursor supplies the observed and
  // expected Codes as a hint without exposing the concrete Errors collector.
  Static::Bytes<128> buffer;
  Writer::Textual hint(buffer);
  hint << "Expected lexical token "_view << Code(type).get_semantics()
       << " but got "_view << current().get_code().get_semantics() << "."_view;
  if (message.is_empty()) {
    create_token_error(hint);
  } else {
    create_token_error(message, hint);
  }
  return Token();
}

auto Cursor::recover_to_statement(View::Vector<Code::Type> terminals) -> void {
  while (get_code() != Code::Type::Terminal &&
         !get_code().is_one_of(terminals)) {
    consume();
  }
  consume();
}

auto Cursor::recover_to_scoped_statement() -> void {
  while (!is_one_of(
      {{Code::Type::Terminal, Code::Type::EndStatement,
        Code::Type::ScopeEnd}})) {
    consume();
  }
  if (matches(Code::Type::EndStatement)) {
    consume();
  }
}

auto tetrodotoxin_source_cursor_representation() -> const ttx_representation* {
  return &Ttx::Semantic::Negotiation::Binding::representation<Cursor>();
}
