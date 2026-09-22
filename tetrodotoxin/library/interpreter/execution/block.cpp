// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/execution/block.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/execution/branch.hpp"
#include "tetrodotoxin/library/interpreter/execution/local.hpp"
#include "tetrodotoxin/library/interpreter/execution/loop_control.hpp"
#include "tetrodotoxin/library/interpreter/execution/match.hpp"
#include "tetrodotoxin/library/interpreter/execution/range_loop.hpp"
#include "tetrodotoxin/library/interpreter/execution/return.hpp"
#include "tetrodotoxin/library/interpreter/expression.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

static auto parse_statement(
    Cursor& cursor,
    Language::Flow::Block& block,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope,
    const Tetrodotoxin::Source::Documentation& documentation,
    Option<const Interpreter::Execution::StatementParser&> extension)
    -> Option<Language::Statement> {
  switch (cursor.get_code().get_type()) {
  case Code::Type::State:
  case Code::Type::Const: {
    auto local = Interpreter::Execution::Local::parse(cursor, block);
    BAIL_IF(!local);
    return Language::Statement::create(
        *local, documentation, local->get_anchor(),
        [](Language::Flow::Local& selected, Cursor& operation_cursor,
           Language::Flow::Scope& scope) {
          return selected.link(operation_cursor, scope.get_access_scope());
        },
        [](Language::Flow::Local& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        },
        [](const Language::Flow::Local&) { return True; },
        [](const Language::Flow::Local& selected) -> Option<View::Bytes> {
          return selected.get_name();
        },
        [](const Language::Flow::Local& selected) -> Option<const Abstract&> {
          return selected.get_linked_type() ? Option<const Abstract&>(selected)
                                            : Option<const Abstract&>();
        });
  }
  case Code::Type::Return: {
    auto returned = Interpreter::Execution::Return::parse(cursor, block);
    BAIL_IF(!returned);
    return Language::Statement::create(
        *returned, documentation, returned->get_anchor(),
        [](Language::Flow::Return& selected, Cursor& operation_cursor,
           Language::Flow::Scope& scope) {
          return selected.link(
              operation_cursor, scope, scope.get_access_scope(),
              scope.get_function_results());
        },
        [](Language::Flow::Return& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        },
        [](const Language::Flow::Return&) { return False; });
  }
  case Code::Type::Break:
  case Code::Type::Continue: {
    auto control = Interpreter::Execution::LoopControl::parse(cursor, block);
    BAIL_IF(!control);
    return Language::Statement::create(
        *control, documentation, control->get_anchor(),
        [](Language::Flow::LoopControl&, Cursor&, Language::Flow::Scope&) {
          return True;
        },
        [](Language::Flow::LoopControl&, Cursor&) {},
        [](const Language::Flow::LoopControl&) { return False; });
  }
  case Code::Type::If:
  case Code::Type::While: {
    auto branch = Interpreter::Execution::Branch::parse(
        cursor, block, function, access_scope, extension);
    BAIL_IF(!branch);
    return Language::Statement::create(
        *branch, documentation, branch->get_anchor(),
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
        });
  }
  case Code::Type::For: {
    auto loop = Interpreter::Execution::RangeLoop::parse(
        cursor, block, function, access_scope, extension);
    BAIL_IF(!loop);
    return Language::Statement::create(
        *loop, documentation, loop->get_anchor(),
        [](Language::Flow::RangeLoop& selected, Cursor& operation_cursor,
           Language::Flow::Scope& scope) {
          return selected.link(operation_cursor, scope.get_access_scope());
        },
        [](Language::Flow::RangeLoop& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        });
  }
  case Code::Type::Match: {
    auto match = Interpreter::Execution::Match::parse(
        cursor, block, function, access_scope, extension);
    BAIL_IF(!match);
    return Language::Statement::create(
        *match, documentation, match->get_anchor(),
        [](Language::Flow::Match& selected, Cursor& operation_cursor,
           Language::Flow::Scope& scope) {
          return selected.link(
              operation_cursor, scope, scope.get_access_scope());
        },
        [](Language::Flow::Match& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        },
        [](const Language::Flow::Match& selected) {
          return selected.reaches_next_statement();
        });
  }
  case Code::Type::ScopeStart: {
    auto nested = Interpreter::Execution::Block::parse(
        cursor, block, function, access_scope,
        block.get_enclosing_loop().visit(
            []() -> Option<Reference<const Abstract>> { return {}; },
            [](const Abstract& loop) -> Option<Reference<const Abstract>> {
              return Reference<const Abstract>(loop);
            }),
        extension);
    BAIL_IF(!nested);
    return Language::Statement::create(
        *nested, documentation, nested->get_anchor(),
        [](Language::Flow::Block& selected, Cursor& operation_cursor,
           Language::Flow::Scope&) { return selected.link(operation_cursor); },
        [](Language::Flow::Block& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        },
        [](const Language::Flow::Block& selected) {
          return selected.reaches_next_statement();
        });
  }
  default:
    break;
  }

  if (extension && extension->matches(cursor)) {
    return extension->parse(
        cursor, block, function, access_scope, documentation);
  }

  Token opening = cursor.current();
  auto expression = Interpreter::Expression::parse(block, cursor);
  BAIL_IF(!expression);
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library expression statements require one terminating `;`."_view);
  BAIL_IF(!terminator);
  return Language::Statement::create_pack(
      *expression, documentation,
      Anchor::create(opening, Span(opening, terminator)),
      [](Language::Model::Pack& selected, Cursor& operation_cursor,
         Language::Flow::Scope& scope) {
        return selected.link(operation_cursor, scope, scope.get_access_scope());
      },
      [](Language::Model::Pack& selected, Cursor& operation_cursor) {
        selected.finalize(operation_cursor);
      });
}

auto Interpreter::Execution::Block::parse(
    Cursor& cursor,
    const Abstract& lexical_context,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope,
    Option<Reference<const Abstract>> enclosing_loop,
    Option<const StatementParser&> extension)
    -> Option<Language::Flow::Block&> {
  Code::Type opening_code = cursor.get_code().get_type();
  if (opening_code != Code::Type::ScopeStart &&
      opening_code != Code::Type::Define) {
    cursor.create_token_error(
        "Library Blocks require `{` for several Statements or `:` for one "
        "Statement."_view);
    return {};
  }
  Token opening = cursor.consume();
  Bool single = opening_code == Code::Type::Define;
  Language::Flow::Block& block = Language::Flow::Block::create_authored(
      cursor.get_arena(), lexical_context, function, access_scope,
      enclosing_loop);

  while (single || !cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          single
              ? "Single Statement Block requires one Statement after `:`."_view
              : "Library Block reached the end of source before `}`."_view);
      Token ending = cursor.peek(-1);
      block.complete_authored(Anchor::create(opening, Span(opening, ending)));
      return block;
    }

    Token statement_start = cursor.current();
    const Tetrodotoxin::Source::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    if (cursor.matches(Code::Type::ScopeEnd)) {
      cursor.create_token_error(
          statement_start,
          documentation.is_empty()
              ? "Single Statement Block requires one Statement after `:`."_view
              : "Library Statement Documentation requires one following form."_view,
          documentation.is_empty()
              ? View::Bytes()
              : "Move this comment before the Statement it describes."_view);
      break;
    }

    auto statement = parse_statement(
        cursor, block, function, access_scope, documentation, extension);
    if (!statement) {
      cursor.recover_to_scoped_statement();
      if (single) {
        break;
      }
      continue;
    }
    block.retain_authored_statement(*statement);
    if (single) {
      break;
    }
  }

  Token closing = single || !cursor.matches(Code::Type::ScopeEnd)
                      ? cursor.peek(-1)
                      : cursor.consume();
  block.complete_authored(Anchor::create(opening, Span(opening, closing)));
  return block;
}
