// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/access/postfix.hpp"

#include "tetrodotoxin/library/language/access/propagate.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/access/unwrap.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Access::Postfix::parse_type(
    const Abstract&,
    Cursor& cursor,
    Language::Model::Pack& receiver) -> Option<Language::Expression&> {
  Token operation = cursor.consume();
  if (!cursor.is_one_of({{Code::Type::Type, Code::Type::Addressable}})) {
    cursor.create_token_error(
        cursor.current(),
        "Type access requires one semantic name after `::`."_view);
    return {};
  }
  Token type = cursor.consume();
  auto receiver_anchor = receiver.get_anchor();
  if (!receiver_anchor) {
    cursor.create_expression_error(
        Anchor::create(type, Span(operation, type)),
        "Type access requires an authored receiver Anchor."_view);
    return {};
  }
  return Language::Access::Type::create_authored(
      cursor.get_arena(), receiver, type,
      type.caculate_text(cursor.get_source_text()),
      Anchor::create(type, receiver_anchor->get_span(), Span(type)));
}

auto Interpreter::Access::Postfix::parse_propagate(
    const Abstract&,
    Cursor& cursor,
    Language::Model::Pack& receiver) -> Option<Language::Expression&> {
  Token operation = cursor.require(
      Code::Type::QuestionOp, "Library propagation requires postfix `?`."_view);
  BAIL_IF(!operation);
  auto receiver_anchor = receiver.get_anchor();
  BAIL_IF(!receiver_anchor);
  return Language::Access::Propagate::create_authored(
      cursor.get_arena(), receiver,
      Anchor::create(operation, receiver_anchor->get_span(), Span(operation)));
}

auto Interpreter::Access::Postfix::parse_unwrap(
    const Abstract&,
    Cursor& cursor,
    Language::Model::Pack& receiver) -> Option<Language::Expression&> {
  Token operation = cursor.require(
      Code::Type::NotOp, "Library Option unwrap requires postfix `!`."_view);
  BAIL_IF(!operation);
  auto receiver_anchor = receiver.get_anchor();
  BAIL_IF(!receiver_anchor);
  return Language::Access::Unwrap::create_authored(
      cursor.get_arena(), receiver,
      Anchor::create(operation, receiver_anchor->get_span(), Span(operation)));
}
