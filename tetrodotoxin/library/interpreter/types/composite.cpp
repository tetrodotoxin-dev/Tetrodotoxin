// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/types/composite.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/member.hpp"

using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Types::Composite::parse_body(
    Cursor& cursor,
    Language::Types::Composite& structure,
    Tetrodotoxin::Language::Definition& definition,
    Token kind_token) -> ParseState {
  if (!cursor.require(
          Code::Type::ScopeStart,
          "Library Composite bodies require an opening `{`."_view)) {
    return ParseState::Incomplete;
  }

  // The real Type hosts each member as soon as its qualifier selects a
  // category. Later declarations can therefore observe stable names even when
  // one earlier edge remains unresolved.
  ParseState state = ParseState::Accepted;
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Library Composite body reached the end of source before `}`."_view);
      return ParseState::Rejected;
    }

    const Tetrodotoxin::Source::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto nested = Tetrodotoxin::Language::Definition::parse(
        cursor, documentation, structure);
    if (!nested) {
      state = ParseState::Rejected;
      cursor.recover_to_scoped_statement();
      continue;
    }
    auto member = Interpreter::Member::parse(cursor, *nested);
    if (!member) {
      state = ParseState::Rejected;
      cursor.recover_to_scoped_statement();
      continue;
    }
    if (!structure.retain_authored_definition(
            member->get_semantic(), *nested, member->get_category(), cursor)) {
      state = ParseState::Rejected;
    }
    if (!member->is_accepted()) {
      state = ParseState::Rejected;
    }
    if (member->needs_recovery()) {
      cursor.recover_to_scoped_statement();
    }
  }

  Token closing = cursor.consume();
  auto& authored = definition.get_authored();
  authored.set_anchor(Anchor::create(
      kind_token, Span(authored.get_anchor().get_span().get_start(), closing)));
  structure.complete_body();
  return state;
}
