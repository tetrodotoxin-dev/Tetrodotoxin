// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// Expression consumes one complete Library value flow operand. Parentheses may
// therefore produce an empty Pack, a named Pack, or a Pack with several values
// without inventing a carrier Expression. Postfix Access and scalar Operations
// prove the narrower Expression category only when their own grammar requires
// it.
class Expression {
 public:
  Expression() = delete;

  static auto parse(
      const Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::Model::Pack&>;

  // Parses one tighter operand in the caller's expression production. Keeping
  // its diagnostics on that Cursor preserves the exact nested failure.
  static auto parse_operand(
      const Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Source::Lexical::Code::Type operation)
      -> Perimortem::Core::Option<Language::Model::Pack&>;

  // The three write operators are the sole right associative binary family.
  // Parsing their right side at the same precedence preserves the authored
  // tree while each empty result makes chained writes semantically invalid.
  static auto parse_write_operand(
      const Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::Model::Pack&>;

  // Parses one prefix operand. Postfix operations remain inside the operand
  // while binary operations remain outside it.
  static auto parse_prefix_operand(
      const Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Source::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::Model::Pack&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter

// Scalar Operations accept Pack operands at their grammar boundary, then
// retain the exact Expression or Constant identities proved here. A
// parenthesized single positional value is already that semantic fact. Named
// Packs and Packs with
// several values remain honest and require an operation that defines their
// shape.
#define TTX_BINARY_PARSE(type, token_type)                                 \
  auto Tetrodotoxin::Library::Language::Operations::type::parse(           \
      const Tetrodotoxin::Source::Abstract& context, Tetrodotoxin::Source::Lexical::Cursor& cursor, \
      Tetrodotoxin::Library::Language::Model::Pack& left,                  \
      Tetrodotoxin::Source::Lexical::Span left_span)                                        \
      -> Perimortem::Core::Option<                                         \
          Tetrodotoxin::Library::Language::Expression&> {                  \
    Tetrodotoxin::Source::Lexical::Token opening = cursor.consume();                        \
    Tetrodotoxin::Source::Lexical::Token right_start = cursor.current();                    \
    Count error_count = cursor.get_error_count();                          \
    auto right =                                                           \
        Tetrodotoxin::Library::Interpreter::Expression::parse_operand(     \
            context, cursor, Tetrodotoxin::Source::Lexical::Code::Type::token_type);        \
    if (!right) {                                                          \
      if (cursor.get_error_count() == error_count) {                       \
        cursor.create_expression_error(                                    \
            Tetrodotoxin::Source::Lexical::Anchor::create(Tetrodotoxin::Source::Lexical::Span(opening)),     \
            "Library binary operation requires one right operand."_view,   \
            "Write one complete Expression after this operator."_view);    \
      }                                                                    \
      return {};                                                           \
    }                                                                      \
    Tetrodotoxin::Source::Lexical::Span right_span(right_start, cursor.peek(-1));           \
    if (!left.get_identity() || !right->get_identity()) {                  \
      cursor.create_expression_error(                                      \
          Tetrodotoxin::Source::Lexical::Anchor::create(opening, left_span, right_span),    \
          "Library scalar operation requires one semantic fact from each " \
          "operand Pack."_view,                                            \
          "Use one unlabelled scalar value; named and multi-value Packs "  \
          "require an operation that defines their shape."_view);          \
      return {};                                                           \
    }                                                                      \
    auto anchor =                                                          \
        Tetrodotoxin::Source::Lexical::Anchor::create(opening, left_span, right_span);      \
    return create_authored(cursor.get_arena(), left, *right, anchor);      \
  }

#define TTX_UNARY_PARSE(type)                                                 \
  auto Tetrodotoxin::Library::Language::Operations::type::parse(              \
      const Tetrodotoxin::Source::Abstract& context, Tetrodotoxin::Source::Lexical::Cursor& cursor)    \
      -> Perimortem::Core::Option<                                            \
          Tetrodotoxin::Library::Language::Expression&> {                     \
    Tetrodotoxin::Source::Lexical::Token opening = cursor.consume();                           \
    Tetrodotoxin::Source::Lexical::Token operand_start = cursor.current();                     \
    Count error_count = cursor.get_error_count();                             \
    auto operand =                                                            \
        Tetrodotoxin::Library::Interpreter::Expression::parse_prefix_operand( \
            context, cursor);                                                 \
    if (!operand) {                                                           \
      if (cursor.get_error_count() == error_count) {                          \
        cursor.create_expression_error(                                       \
            Tetrodotoxin::Source::Lexical::Anchor::create(Tetrodotoxin::Source::Lexical::Span(opening)),        \
            "Library prefix operation requires one operand."_view,            \
            "Write one complete Expression after this operator."_view);       \
      }                                                                       \
      return {};                                                              \
    }                                                                         \
    Tetrodotoxin::Source::Lexical::Span operand_span(operand_start, cursor.peek(-1));          \
    if (!operand->get_identity()) {                                           \
      cursor.create_expression_error(                                         \
          Tetrodotoxin::Source::Lexical::Anchor::create(                                       \
              opening, Tetrodotoxin::Source::Lexical::Span(opening), operand_span),            \
          "Library scalar operation requires one semantic operand "           \
          "Pack."_view,                                                       \
          "Use one unlabelled scalar value; named and multi-value Packs "     \
          "require an operation that defines their shape."_view);             \
      return {};                                                              \
    }                                                                         \
    auto anchor = Tetrodotoxin::Source::Lexical::Anchor::create(                               \
        opening, Tetrodotoxin::Source::Lexical::Span(opening), operand_span);                  \
    return create_authored(cursor.get_arena(), *operand, anchor);             \
  }
