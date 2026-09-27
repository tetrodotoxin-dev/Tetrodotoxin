// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/access/slice.hpp"

#include "tetrodotoxin/library/interpreter/expression.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

static auto complete_postfix_span(Cursor& cursor, Token opening) -> Span {
  while (!cursor.matches(Code::Type::Terminal) &&
         !cursor.matches(Code::Type::BracketEnd)) {
    cursor.consume();
  }
  if (cursor.matches(Code::Type::BracketEnd)) {
    cursor.consume();
  }
  return Span(opening, cursor.peek(-1));
}

static auto reject_syntax(Cursor& cursor, Span span) -> void {
  cursor.create_expression_error(
      span, "Slice access has malformed index or range operands."_view,
      "Use `:[index]` or `:[start, count]` with complete delimiters."_view);
}

static auto reject_operand(Cursor& cursor, Span postfix_span, Span operand_span)
    -> void {
  auto report = cursor.create_report(postfix_span);
  report << "Slice access operand `"_view
         << operand_span.caculate_text(cursor.get_source_text())
         << "` could not be parsed as a complete Expression."_view;
  report.get_hint() << "Use a complete scalar or byte Expression."_view;
}

static auto parse_operand(const Abstract& context, Cursor& cursor)
    -> Core::Option<Language::Model::Pack&> {
  return Interpreter::Expression::parse(context, cursor);
}

auto Interpreter::Access::Slice::parse(
    const Abstract& context,
    Cursor& cursor,
    Language::Model::Pack& receiver) -> Core::Option<Language::Expression&> {
  Token opening = cursor.consume();
  Code first_code = cursor.get_code();
  if (first_code.is_one_of({{
        Code::Type::Terminal,
        Code::Type::PackingOp,
        Code::Type::BracketEnd,
      }})) {
    Span span = complete_postfix_span(cursor, opening);
    reject_syntax(cursor, span);
    return {};
  }

  Token first_start = cursor.current();
  Token first_end =
      cursor.matches(Code::Type::SubOp) ? cursor.peek(1) : first_start;
  auto first = parse_operand(context, cursor);
  if (!first) {
    Span span = complete_postfix_span(cursor, opening);
    reject_operand(cursor, span, Span(first_start, first_end));
    return {};
  }

  Bool range = cursor.matches(Code::Type::PackingOp);
  Core::Option<Language::Model::Pack&> second;
  if (range) {
    cursor.consume();
    Code second_code = cursor.get_code();
    if (second_code.is_one_of({{
          Code::Type::Terminal,
          Code::Type::PackingOp,
          Code::Type::BracketEnd,
        }})) {
      Span span = complete_postfix_span(cursor, opening);
      reject_syntax(cursor, span);
      return {};
    }

    Token second_start = cursor.current();
    Token second_end =
        cursor.matches(Code::Type::SubOp) ? cursor.peek(1) : second_start;
    second = parse_operand(context, cursor);
    if (!second) {
      Span span = complete_postfix_span(cursor, opening);
      reject_operand(cursor, span, Span(second_start, second_end));
      return {};
    }
  }

  if (!cursor.matches(Code::Type::BracketEnd)) {
    Span span = complete_postfix_span(cursor, opening);
    reject_syntax(cursor, span);
    return {};
  }
  cursor.consume();
  Token closing = cursor.peek(-1);
  const auto& receiver_anchor = receiver.get_anchor();
  const auto& first_anchor = first->get_anchor();
  if (!receiver_anchor || !first_anchor ||
      (range && (!second || !second->get_anchor()))) {
    cursor.create_expression_error(
        Span(opening, closing),
        "Slice access requires authored operand Anchors."_view);
    return {};
  }

  Anchor anchor =
      Anchor::create(opening, receiver_anchor->get_span(), Span(closing));
  return range ? Language::Access::Slice::create_authored(
                     cursor.get_arena(), receiver, *first, *second, anchor)
               : Language::Access::Slice::create_authored(
                     cursor.get_arena(), receiver, *first, anchor);
}
