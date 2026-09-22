// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/execution/branch.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/execution/block.hpp"
#include "tetrodotoxin/library/interpreter/pack.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Execution::Branch::parse(
    Cursor& cursor,
    Language::Flow::Block& lexical_context,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope,
    Option<const StatementParser&> extension)
    -> Option<Language::Flow::Branch&> {
  Token opening = cursor.current();
  Language::Flow::Branch::Kind kind;
  switch (cursor.get_code().get_type()) {
  case Code::Type::If:
    kind = Language::Flow::Branch::Kind::If;
    break;
  case Code::Type::While:
    kind = Language::Flow::Branch::Kind::While;
    break;
  default:
    cursor.create_token_error(
        "Library branches require the `if` or `while` keyword."_view);
    return {};
  }
  cursor.consume();

  auto condition = Interpreter::Pack::parse(lexical_context, cursor);
  BAIL_IF(!condition);
  Language::Flow::Branch& result = Language::Flow::Branch::create_authored(
      cursor.get_arena(), kind, *condition,
      Anchor::create(opening, Span(opening, cursor.peek(-1))));

  Option<Reference<const Abstract>> enclosing_loop;
  if (kind == Language::Flow::Branch::Kind::While) {
    enclosing_loop = Reference<const Abstract>(result);
  } else {
    auto inherited = lexical_context.get_enclosing_loop();
    if (inherited) {
      enclosing_loop = Reference<const Abstract>(*inherited);
    }
  }

  auto body = Block::parse(
      cursor, lexical_context, function, access_scope, enclosing_loop,
      extension);
  BAIL_IF(!body || !result.complete_body(*body));
  if (kind == Language::Flow::Branch::Kind::If &&
      cursor.matches(Code::Type::Else)) {
    cursor.consume();
    const Tetrodotoxin::Source::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    if (cursor.matches(Code::Type::If)) {
      auto nested =
          parse(cursor, lexical_context, function, access_scope, extension);
      BAIL_IF(!nested);
      BAIL_IF(!result.complete_alternate(
          Language::Statement::create(
              *nested, documentation, nested->get_anchor(),
              [](Language::Flow::Branch& selected, Cursor& operation_cursor,
                 Language::Flow::Scope& scope) {
                return selected.link(
                    operation_cursor, scope, scope.get_access_scope());
              },
              [](Language::Flow::Branch& selected, Cursor& operation_cursor) {
                selected.finalize(operation_cursor);
              },
              [](const Language::Flow::Branch& selected) {
                return selected.reaches_next_statement();
              })));
    } else if (
        cursor.matches(Code::Type::ScopeStart) ||
        cursor.matches(Code::Type::Define)) {
      auto nested = Block::parse(
          cursor, lexical_context, function, access_scope, enclosing_loop,
          extension);
      BAIL_IF(!nested);
      BAIL_IF(!result.complete_alternate(
          Language::Statement::create(
              *nested, documentation, nested->get_anchor(),
              [](Language::Flow::Block& selected, Cursor& operation_cursor,
                 Language::Flow::Scope&) {
                return selected.link(operation_cursor);
              },
              [](Language::Flow::Block& selected, Cursor& operation_cursor) {
                selected.finalize(operation_cursor);
              },
              [](const Language::Flow::Block& selected) {
                return selected.reaches_next_statement();
              })));
    } else {
      cursor.create_token_error(
          "Library `else` requires one nested `if` or Block beginning with "
          "`{` or `:`."_view);
      return {};
    }
  }

  result.complete_anchor(
      Anchor::create(opening, Span(opening, cursor.peek(-1))));
  return result;
}
