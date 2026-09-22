// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/operation.hpp"

#include "tetrodotoxin/library/interpreter/expression.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/operations/add_assignment.hpp"
#include "tetrodotoxin/library/language/operations/and.hpp"
#include "tetrodotoxin/library/language/operations/assignment.hpp"
#include "tetrodotoxin/library/language/operations/divide.hpp"
#include "tetrodotoxin/library/language/operations/equal.hpp"
#include "tetrodotoxin/library/language/operations/greater.hpp"
#include "tetrodotoxin/library/language/operations/greater_equal.hpp"
#include "tetrodotoxin/library/language/operations/less.hpp"
#include "tetrodotoxin/library/language/operations/less_equal.hpp"
#include "tetrodotoxin/library/language/operations/modulo.hpp"
#include "tetrodotoxin/library/language/operations/multiply.hpp"
#include "tetrodotoxin/library/language/operations/negate.hpp"
#include "tetrodotoxin/library/language/operations/not.hpp"
#include "tetrodotoxin/library/language/operations/not_equal.hpp"
#include "tetrodotoxin/library/language/operations/or.hpp"
#include "tetrodotoxin/library/language/operations/range.hpp"
#include "tetrodotoxin/library/language/operations/subtract.hpp"
#include "tetrodotoxin/library/language/operations/subtract_assignment.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

template <typename operation_type>
static auto parse_scalar_binary(
    Code::Type code,
    const Abstract& context,
    Cursor& cursor,
    Language::Model::Pack& left,
    Span left_span) -> Option<Language::Expression&> {
  Token opening = cursor.consume();
  Token right_start = cursor.current();
  Count error_count = cursor.get_error_count();
  auto right = Interpreter::Expression::parse_operand(context, cursor, code);
  if (!right) {
    if (cursor.get_error_count() == error_count) {
      cursor.create_expression_error(
          Anchor::create(Span(opening)),
          "Library binary operation requires one right operand."_view,
          "Write one complete Expression after this operator."_view);
    }
    return {};
  }

  Span right_span(right_start, cursor.peek(-1));
  if (!left.get_identity() || !right->get_identity()) {
    cursor.create_expression_error(
        Anchor::create(opening, left_span, right_span),
        "Library scalar operation requires one semantic fact from each operand "
        "Pack."_view,
        "Use one unlabelled scalar value because named and multiple value "
        "Packs require an operation that defines their shape."_view);
    return {};
  }

  return operation_type::create_authored(
      cursor.get_arena(), left, *right,
      Anchor::create(opening, left_span, right_span));
}

template <typename operation_type>
static auto parse_scalar_prefix(const Abstract& context, Cursor& cursor)
    -> Option<Language::Expression&> {
  Token opening = cursor.consume();
  Token operand_start = cursor.current();
  Count error_count = cursor.get_error_count();
  auto operand = Interpreter::Expression::parse_prefix_operand(context, cursor);
  if (!operand) {
    if (cursor.get_error_count() == error_count) {
      cursor.create_expression_error(
          Anchor::create(Span(opening)),
          "Library prefix operation requires one operand."_view,
          "Write one complete Expression after this operator."_view);
    }
    return {};
  }
  Span operand_span(operand_start, cursor.peek(-1));
  if (!operand->get_identity()) {
    cursor.create_expression_error(
        Anchor::create(opening, Span(opening), operand_span),
        "Library unary operation requires one semantic operand."_view,
        "Use one unlabelled scalar value after the prefix operator."_view);
    return {};
  }
  return operation_type::create_authored(
      cursor.get_arena(), *operand,
      Anchor::create(opening, Span(opening), operand_span));
}

static auto parse_range(
    const Abstract& context,
    Cursor& cursor,
    Language::Model::Pack& left,
    Span left_span) -> Option<Language::Expression&> {
  Token opening = cursor.consume();
  Token right_start = cursor.current();
  auto right = Interpreter::Expression::parse_operand(
      context, cursor, Code::Type::RangeOp);
  Span span(opening, cursor.peek(-1));
  if (!right) {
    cursor.create_expression_error(
        span, "Range has a malformed right endpoint."_view,
        "Use one complete integer Expression after `...`."_view);
    return {};
  }
  if (cursor.matches(Code::Type::RangeOp)) {
    cursor.create_expression_error(
        Span(opening, cursor.current()),
        "Range accepts exactly two endpoints."_view,
        "Finish one Range before starting another Expression."_view);
    return {};
  }

  Span right_span(right_start, cursor.peek(-1));
  if (!left.get_identity() || !right->get_identity()) {
    cursor.create_expression_error(
        Anchor::create(opening, left_span, right_span),
        "Library Range requires one semantic fact from each endpoint Pack."_view,
        "Use one unlabelled integer value for each Range endpoint."_view);
    return {};
  }
  return Language::Operations::Range::create_authored(
      cursor.get_arena(), left, *right,
      Anchor::create(opening, left_span, right_span));
}

static auto parse_assignment(
    const Abstract& context,
    Cursor& cursor,
    Language::Model::Pack& left,
    Span left_span) -> Option<Language::Expression&> {
  auto target = left.select_identity<Language::Expression>();
  if (!target) {
    cursor.create_expression_error(
        left_span, "Library assignment requires one Expression target."_view,
        "Assign through one writable expression rather than composed Pack "
        "flow."_view);
    return {};
  }

  Token operation = cursor.require(
      Code::Type::Assign,
      "Library Assignment requires the unambiguous `=` operator."_view);
  BAIL_IF(!operation);
  Token right_start = cursor.current();
  Count error_count = cursor.get_error_count();
  auto right = Interpreter::Expression::parse_write_operand(context, cursor);
  if (!right) {
    if (cursor.get_error_count() == error_count) {
      cursor.create_expression_error(
          Anchor::create(Span(operation)),
          "Library assignment requires one right operand."_view,
          "Write one complete value flow after `=`."_view);
    }
    return {};
  }

  return Language::Operations::Assignment::create_authored(
      cursor.get_arena(), *target, *right,
      Anchor::create(operation, left_span, Span(right_start, cursor.peek(-1))));
}

template <typename operation_type>
static auto parse_compound_assignment(
    Code::Type code,
    const Abstract& context,
    Cursor& cursor,
    Language::Model::Pack& left,
    Span left_span) -> Option<Language::Expression&> {
  auto target = left.select_identity<Language::Expression>();
  if (!target) {
    cursor.create_expression_error(
        left_span,
        "Library compound assignment requires one Expression target."_view,
        "Apply this operator through one writable scalar expression."_view);
    return {};
  }

  Token operation = cursor.require(
      code, "Library compound assignment requires its exact operator."_view);
  BAIL_IF(!operation);
  Token right_start = cursor.current();
  Count error_count = cursor.get_error_count();
  auto right_pack =
      Interpreter::Expression::parse_write_operand(context, cursor);
  if (!right_pack) {
    if (cursor.get_error_count() == error_count) {
      cursor.create_expression_error(
          Anchor::create(Span(operation)),
          "Library compound assignment requires one right operand."_view,
          "Write one scalar value after this operator."_view);
    }
    return {};
  }

  Anchor anchor =
      Anchor::create(operation, left_span, Span(right_start, cursor.peek(-1)));
  if (!right_pack->get_identity()) {
    cursor.create_expression_error(
        anchor, "Library compound assignment requires one semantic value."_view,
        "Use one unlabelled scalar operand for this operator."_view);
    return {};
  }
  return operation_type::create_authored(
      cursor.get_arena(), *target, *right_pack, anchor);
}

auto Interpreter::Operation::parse_binary(
    Code::Type code,
    const Abstract& context,
    Cursor& cursor,
    Language::Model::Pack& left,
    Span left_span) -> Option<Language::Expression&> {
  switch (code) {
  case Code::Type::DivOp:
    return parse_scalar_binary<Language::Operations::Divide>(
        code, context, cursor, left, left_span);
  case Code::Type::ModOp:
    return parse_scalar_binary<Language::Operations::Modulo>(
        code, context, cursor, left, left_span);
  case Code::Type::MulOp:
    return parse_scalar_binary<Language::Operations::Multiply>(
        code, context, cursor, left, left_span);
  case Code::Type::AddOp:
    return parse_scalar_binary<Language::Operations::Add>(
        code, context, cursor, left, left_span);
  case Code::Type::SubOp:
    return parse_scalar_binary<Language::Operations::Subtract>(
        code, context, cursor, left, left_span);
  case Code::Type::LessOp:
    return parse_scalar_binary<Language::Operations::Less>(
        code, context, cursor, left, left_span);
  case Code::Type::GreaterOp:
    return parse_scalar_binary<Language::Operations::Greater>(
        code, context, cursor, left, left_span);
  case Code::Type::GreaterEqOp:
    return parse_scalar_binary<Language::Operations::GreaterEqual>(
        code, context, cursor, left, left_span);
  case Code::Type::LessEqOp:
    return parse_scalar_binary<Language::Operations::LessEqual>(
        code, context, cursor, left, left_span);
  case Code::Type::CmpOp:
    return parse_scalar_binary<Language::Operations::Equal>(
        code, context, cursor, left, left_span);
  case Code::Type::NotEqOp:
    return parse_scalar_binary<Language::Operations::NotEqual>(
        code, context, cursor, left, left_span);
  case Code::Type::And:
    return parse_scalar_binary<Language::Operations::And>(
        code, context, cursor, left, left_span);
  case Code::Type::Or:
    return parse_scalar_binary<Language::Operations::Or>(
        code, context, cursor, left, left_span);
  case Code::Type::RangeOp:
    return parse_range(context, cursor, left, left_span);
  case Code::Type::Assign:
    return parse_assignment(context, cursor, left, left_span);
  case Code::Type::AddAssign:
    return parse_compound_assignment<Language::Operations::AddAssignment>(
        code, context, cursor, left, left_span);
  case Code::Type::SubAssign:
    return parse_compound_assignment<Language::Operations::SubtractAssignment>(
        code, context, cursor, left, left_span);
  default:
    return {};
  }
}

auto Interpreter::Operation::parse_prefix(
    Code::Type code,
    const Abstract& context,
    Cursor& cursor) -> Option<Language::Expression&> {
  switch (code) {
  case Code::Type::NotOp:
    return parse_scalar_prefix<Language::Operations::Not>(context, cursor);
  case Code::Type::SubOp:
    return parse_scalar_prefix<Language::Operations::Negate>(context, cursor);
  default:
    return {};
  }
}
