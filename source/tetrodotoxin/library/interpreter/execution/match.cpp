// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/execution/match.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/execution/block.hpp"
#include "tetrodotoxin/library/interpreter/expression.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Execution::Match::parse(
    Cursor& cursor,
    Language::Flow::Block& lexical_context,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope,
    Option<const StatementParser&> extension)
    -> Option<Language::Flow::Match&> {
  Token opening = cursor.require(
      Code::Type::Match,
      "Library match statements require the `match` keyword."_view);
  BAIL_IF(!opening);
  Token input_opening = cursor.current();
  auto input_pack = Interpreter::Expression::parse(lexical_context, cursor);
  BAIL_IF(!input_pack);
  if (!input_pack->get_identity()) {
    cursor.create_expression_error(
        Span(input_opening, cursor.peek(-1)),
        "Library match input must be one scalar Expression."_view,
        "Use one unlabelled value instead of empty, named, or composed Pack "
        "flow."_view);
    return {};
  }

  Token scope_opening = cursor.require(
      Code::Type::ScopeStart,
      "Library match cases require a body beginning with `{`."_view);
  BAIL_IF(!scope_opening);
  Language::Flow::Match& result = Language::Flow::Match::create_authored(
      cursor.get_arena(), *input_pack,
      Anchor::create(opening, Span(opening, scope_opening)));
  Option<Reference<const Abstract>> enclosing_loop;
  auto inherited = lexical_context.get_enclosing_loop();
  if (inherited) {
    enclosing_loop = Reference<const Abstract>(*inherited);
  }

  Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    Token case_token = cursor.require(
        Code::Type::Case,
        "Library match bodies contain only authored `case` forms."_view);
    BAIL_IF(!case_token);
    if (cursor.matches(Code::Type::Discard)) {
      cursor.consume();
      auto body = Block::parse(
          cursor, lexical_context, function, access_scope, enclosing_loop,
          extension);
      BAIL_IF(!body || !result.complete_default(*body));
      Tetrodotoxin::Language::Parser::Comment::parse(cursor);
      if (!cursor.matches(Code::Type::ScopeEnd)) {
        cursor.create_token_error(
            "The `_` Library match case must be final."_view);
        return {};
      }
      continue;
    }

    if (cursor.matches(Code::Type::Addressable) &&
        (cursor.peek(1).get_code().get_type() == Code::Type::Define ||
         cursor.peek(1).get_code().get_type() == Code::Type::ScopeStart)) {
      Token value_token = cursor.consume();
      auto pattern = Language::Flow::Match::create_pattern(
          cursor.get_arena(), lexical_context,
          value_token.caculate_text(cursor.get_source_text()));
      auto& expression = Language::Expressions::Identifier::create_authored(
          cursor, pattern.get_context(), value_token,
          Anchor::create(Span(value_token)));
      auto body = Block::parse(
          cursor, pattern.get_context(), function, access_scope, enclosing_loop,
          extension);
      BAIL_IF(!body);
      result.retain_value_case(
          expression, *body, pattern.get_payload(),
          Anchor::create(Span(value_token)));
      Tetrodotoxin::Language::Parser::Comment::parse(cursor);
      continue;
    }

    Token expression_opening = cursor.current();
    auto case_pack = Interpreter::Expression::parse(lexical_context, cursor);
    BAIL_IF(!case_pack);
    if (!case_pack->get_identity()) {
      cursor.create_expression_error(
          Span(expression_opening, cursor.peek(-1)),
          "Library match case must be one scalar Expression."_view,
          "Use one unlabelled value that can fold to a Constant."_view);
      return {};
    }
    auto body = Block::parse(
        cursor, lexical_context, function, access_scope, enclosing_loop,
        extension);
    BAIL_IF(!body);
    result.retain_constant_case(
        *case_pack, *body,
        case_pack->get_anchor().visit(
            [&]() { return Anchor::create(Span(expression_opening)); },
            [](Anchor selected) { return selected; }));
    Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  }

  Token closing = cursor.consume();
  result.complete_anchor(Anchor::create(opening, Span(opening, closing)));
  return result;
}
