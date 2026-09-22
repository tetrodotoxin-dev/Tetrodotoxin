// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/access/index.hpp"

#include "tetrodotoxin/library/interpreter/expression.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

static auto parse_index(const Abstract& context, Cursor& cursor)
    -> Option<Language::Model::Pack&> {
  return Interpreter::Expression::parse(context, cursor);
}

auto Interpreter::Access::Index::parse(
    const Abstract& context,
    Cursor& cursor,
    Language::Model::Pack& receiver) -> Option<Language::Expression&> {
  Token opening = cursor.require(
      Code::Type::BracketStart,
      "Index reference access requires an opening `[`."_view);
  BAIL_IF(!opening);
  auto selected = parse_index(context, cursor);
  if (!selected) {
    cursor.create_expression_error(
        Span(opening, cursor.current()),
        "Index access requires one complete first Expression."_view,
        "Use `[index]` or `[start, count]` with integer Expressions."_view);
    return {};
  }

  Option<Language::Model::Pack&> count;
  if (cursor.matches(Code::Type::PackingOp)) {
    cursor.consume();
    count = parse_index(context, cursor);
    if (!count) {
      cursor.create_expression_error(
          Span(opening, cursor.current()),
          "Ranged Index access requires one complete count Expression."_view,
          "Use `[start, count]` with an integer count."_view);
      return {};
    }
  }
  if (!cursor.matches(Code::Type::BracketEnd)) {
    cursor.create_expression_error(
        Span(opening, cursor.current()),
        "Index access requires one closing `]`."_view,
        "Use `[index]` or `[start, count]` with complete delimiters."_view);
    return {};
  }

  Token closing = cursor.consume();
  const auto& receiver_anchor = receiver.get_anchor();
  const auto& index_anchor = selected->get_anchor();
  if (!receiver_anchor || !index_anchor || (count && !count->get_anchor())) {
    cursor.create_expression_error(
        Span(opening, closing),
        "Index access requires authored receiver and operand Anchors."_view);
    return {};
  }
  Anchor anchor =
      Anchor::create(opening, receiver_anchor->get_span(), Span(closing));
  return count ? Language::Access::Index::create_authored(
                     cursor.get_arena(), receiver, *selected, *count, anchor)
               : Language::Access::Index::create_authored(
                     cursor.get_arena(), receiver, *selected, anchor);
}
