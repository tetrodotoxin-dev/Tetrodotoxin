// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/expression.hpp"

#include "tetrodotoxin/library/interpreter/access/address.hpp"
#include "tetrodotoxin/library/interpreter/access/call.hpp"
#include "tetrodotoxin/library/interpreter/access/index.hpp"
#include "tetrodotoxin/library/interpreter/access/postfix.hpp"
#include "tetrodotoxin/library/interpreter/access/slice.hpp"
#include "tetrodotoxin/library/interpreter/access/swizzle.hpp"
#include "tetrodotoxin/library/interpreter/expressions/initializer.hpp"
#include "tetrodotoxin/library/interpreter/literal.hpp"
#include "tetrodotoxin/library/interpreter/operation.hpp"
#include "tetrodotoxin/library/interpreter/pack.hpp"
#include "tetrodotoxin/library/language/access/propagate.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/access/unwrap.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;

// Prefix operators keep Slice access inside their operand and stop before
// multiplicative grammar, so concrete unary owners never replay either level.
static constexpr Count prefix_precedence = 31;

static auto associate_pack(Cursor& cursor, Library::Language::Model::Pack& pack)
    -> void {
  auto identity = pack.get_identity();
  auto anchor = pack.get_anchor();
  if (!identity || !anchor) {
    return;
  }

  // A malformed following postfix can prevent the completed access node from
  // entering a Statement, but the receiver spelling still asks one factual
  // Library question. Publish the exact selected identity while the concrete
  // parser still owns its lexical context; generic tooling then needs only the
  // shared TTX queries on that Association.
  const Abstract& authored =
      identity->visit<Library::Language::Expressions::Identifier>(
          [](const Library::Language::Expressions::Identifier& identifier)
              -> const Abstract& { return identifier.resolve_authored(); },
          [](const Abstract& candidate) -> const Abstract& {
            return candidate.visit<Library::Language::Access::Type>(
                [](const Library::Language::Access::Type& access)
                    -> const Abstract& { return access.resolve_authored(); },
                [](const Abstract& direct) -> const Abstract& {
                  return direct;
                });
          });
  const Abstract& semantic =
      authored.is<Unknown>() || authored.is<None>() ? *identity : authored;
  cursor.get_associations().create(*anchor, semantic);
}

static auto is_postfix(Code::Type code) -> Bool {
  switch (code) {
  case Code::Type::AddressOp:
  case Code::Type::CallOp:
  case Code::Type::BracketStart:
  case Code::Type::TypeAccessOp:
  case Code::Type::ValueAccessOp:
  case Code::Type::NotOp:
  case Code::Type::QuestionOp:
    return True;
  default:
    return False;
  }
}

static auto parse_postfix(
    Code::Type code,
    const Abstract& context,
    Cursor& cursor,
    Library::Language::Model::Pack& receiver)
    -> Option<Library::Language::Expression&> {
  switch (code) {
  case Code::Type::AddressOp:
    return Library::Interpreter::Access::Address::parse(
        context, cursor, receiver);
  case Code::Type::CallOp:
    return Library::Interpreter::Access::Call::parse(context, cursor, receiver);
  case Code::Type::BracketStart:
    return Library::Interpreter::Access::Index::parse(
        context, cursor, receiver);
  case Code::Type::TypeAccessOp:
    return Library::Interpreter::Access::Postfix::parse_type(
        context, cursor, receiver);
  case Code::Type::ValueAccessOp:
    return Library::Interpreter::Access::Slice::parse(
        context, cursor, receiver);
  case Code::Type::NotOp:
    return Library::Interpreter::Access::Postfix::parse_unwrap(
        context, cursor, receiver);
  case Code::Type::QuestionOp:
    return Library::Interpreter::Access::Postfix::parse_propagate(
        context, cursor, receiver);
  default:
    return {};
  }
}

static auto get_precedence(Code::Type code) -> Count {
  switch (code) {
  case Code::Type::DivOp:
  case Code::Type::ModOp:
  case Code::Type::MulOp:
    return 30;
  case Code::Type::AddOp:
  case Code::Type::SubOp:
    return 20;
  case Code::Type::LessOp:
  case Code::Type::GreaterOp:
  case Code::Type::GreaterEqOp:
  case Code::Type::LessEqOp:
    return 10;
  case Code::Type::CmpOp:
  case Code::Type::NotEqOp:
    return 5;
  case Code::Type::And:
    return 4;
  case Code::Type::Or:
    return 3;
  case Code::Type::RangeOp:
    return 2;
  case Code::Type::Assign:
  case Code::Type::AddAssign:
  case Code::Type::SubAssign:
    return 1;
  default:
    return 0;
  }
}

static auto parse_primary(const Abstract& context, Cursor& cursor)
    -> Option<Library::Language::Model::Pack&> {
  if (cursor.matches(Code::Type::PackingStart)) {
    return Library::Interpreter::Pack::parse(context, cursor, True);
  }

  if (Library::Interpreter::Expressions::Initializer::is_next(cursor)) {
    auto initializer =
        Library::Interpreter::Expressions::Initializer::parse(context, cursor);
    BAIL_IF(!initializer);
    return static_cast<Library::Language::Model::Pack&>(*initializer);
  }

  if (cursor.matches(Code::Type::Type) ||
      cursor.matches(Code::Type::Addressable) ||
      cursor.matches(Code::Type::Self) || cursor.matches(Code::Type::Source)) {
    Token token = cursor.consume();
    return Library::Language::Expressions::Identifier::create_authored(
        cursor, context, token, Anchor::create(Span(token)));
  }

  if (cursor.matches(Code::Type::NotOp)) {
    auto operation = Library::Interpreter::Operation::parse_prefix(
        Code::Type::NotOp, context, cursor);
    BAIL_IF(!operation);
    return static_cast<Library::Language::Model::Pack&>(*operation);
  }

  if (cursor.matches(Code::Type::SubOp)) {
    // Literal keeps the sign for decimal and Real spellings. Every other
    // leading subtraction Token enters the general Negate grammar.
    switch (cursor.peek(1).get_code().get_type()) {
    case Code::Type::Numeric:
    case Code::Type::Float: {
      auto literal = Library::Interpreter::Literal::parse(context, cursor);
      BAIL_IF(!literal);
      return *literal;
    }
    default:
      auto operation = Library::Interpreter::Operation::parse_prefix(
          Code::Type::SubOp, context, cursor);
      BAIL_IF(!operation);
      return static_cast<Library::Language::Model::Pack&>(*operation);
    }
  }

  switch (cursor.get_code().get_type()) {
  case Code::Type::String:
  case Code::Type::Bytes:
  case Code::Type::True:
  case Code::Type::False:
  case Code::Type::Numeric:
  case Code::Type::Hex:
  case Code::Type::Float:
  case Code::Type::Embedded: {
    auto literal = Library::Interpreter::Literal::parse(context, cursor);
    BAIL_IF(!literal);
    return *literal;
  }
  default:
    return {};
  }
}

static auto parse_expression(
    const Abstract& context,
    Cursor& cursor,
    Count minimum_precedence) -> Option<Library::Language::Model::Pack&> {
  Token start = cursor.current();
  auto primary = parse_primary(context, cursor);
  BAIL_IF(!primary);
  associate_pack(cursor, *primary);

  Tetrodotoxin::Source::PackReference<Library::Language::Model::Pack> parsed(*primary);
  Span parsed_span(start, cursor.peek(-1));

  // Postfix Access binds to the complete receiver before binary grammar. Each
  // completed node becomes the receiver for the next suffix. The binary loop
  // below then resolves that finished chain as its left Expression.
  while (True) {
    if (cursor.matches(Code::Type::SwizzleOp)) {
      auto selected = Library::Interpreter::Access::Swizzle::parse(
          context, cursor, parsed.get(), parsed_span);
      BAIL_IF(!selected);

      parsed =
          Tetrodotoxin::Source::PackReference<Library::Language::Model::Pack>(*selected);
      parsed_span = Span(start, cursor.peek(-1));
      associate_pack(cursor, parsed.get());
      continue;
    }

    Code::Type postfix = cursor.get_code().get_type();
    if (!is_postfix(postfix)) {
      break;
    }

    if (!parsed.get().get_identity()) {
      cursor.create_expression_error(
          Anchor::create(cursor.current(), parsed_span, Span(cursor.current())),
          "Library postfix access requires one scalar receiver Pack."_view,
          "Select through one unlabelled scalar value; named and multi-value "
          "Packs have no implicit receiver."_view);
      return {};
    }

    auto selected = parse_postfix(postfix, context, cursor, parsed.get());
    BAIL_IF(!selected);

    parsed =
        Tetrodotoxin::Source::PackReference<Library::Language::Model::Pack>(*selected);
    parsed_span = Span(start, cursor.peek(-1));
    associate_pack(cursor, parsed.get());
  }

  while (True) {
    Code::Type binary = cursor.get_code().get_type();
    Count precedence = get_precedence(binary);
    if (precedence == 0 || precedence < minimum_precedence) {
      return parsed.get();
    }

    auto selected = Library::Interpreter::Operation::parse_binary(
        binary, context, cursor, parsed.get(), parsed_span);
    BAIL_IF(!selected);

    parsed =
        Tetrodotoxin::Source::PackReference<Library::Language::Model::Pack>(*selected);
    parsed_span = Span(start, cursor.peek(-1));
    associate_pack(cursor, parsed.get());
  }
}

auto Library::Interpreter::Expression::parse(
    const Abstract& context,
    Cursor& cursor) -> Option<Language::Model::Pack&> {
  Count error_count = cursor.get_error_count();
  auto parsed = parse_expression(context, cursor, 0);
  if (!parsed) {
    if (cursor.get_error_count() == error_count) {
      cursor.create_token_error(
          "Library expression requires one value operand."_view,
          "Write one literal, name, grouped Pack, or prefix expression."_view);
    }
    return {};
  }
  return *parsed;
}

auto Library::Interpreter::Expression::parse_operand(
    const Abstract& context,
    Cursor& cursor,
    Code::Type operation) -> Option<Language::Model::Pack&> {
  Count precedence = get_precedence(operation);
  BAIL_IF(precedence == 0);

  return parse_expression(context, cursor, precedence + 1);
}

auto Library::Interpreter::Expression::parse_write_operand(
    const Abstract& context,
    Cursor& cursor) -> Option<Language::Model::Pack&> {
  return parse_expression(context, cursor, get_precedence(Code::Type::Assign));
}

auto Library::Interpreter::Expression::parse_prefix_operand(
    const Abstract& context,
    Cursor& cursor) -> Option<Language::Model::Pack&> {
  return parse_expression(context, cursor, prefix_precedence);
}
