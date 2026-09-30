// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialect/library/function.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/textual.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/execution/fields/value.hpp"
#include "tetrodotoxin/model/execution/functions/function.hpp"
#include "tetrodotoxin/model/execution/layouts/sequence.hpp"
#include "tetrodotoxin/model/execution/statements/block.hpp"
#include "tetrodotoxin/model/execution/statements/return.hpp"
#include "tetrodotoxin/model/execution/values/literal.hpp"
#include "tetrodotoxin/model/execution/values/parameter.hpp"
#include "tetrodotoxin/source/policies/reference.hpp"
#include "ttx/concept/answers/none.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source::Lexical;

static auto authored(
    Memory::Allocator::Arena& arena,
    Cursor& cursor,
    Abstract subject,
    Token name,
    Tetrodotoxin::Source::Anchor anchor) -> Abstract {
  const auto& policy =
      arena.construct<Tetrodotoxin::Source::Policies::Authored>(
          subject, anchor,
          name ? arena.proxy(cursor.get_text(name)) : Core::View::Bytes());
  const auto view = Abstract::provide(policy);
  return view;
}

static auto fields(
    Memory::Allocator::Arena& arena,
    Cursor& cursor,
    Abstract types,
    Bool parameters,
    Memory::Managed::Vector<Abstract>& output) -> Bool {
  BAIL_IF(!cursor.require(Code::Type::BracketStart));
  while (!cursor.matches(Code::Type::BracketEnd)) {
    Token opening = cursor.current();
    Token name;
    if (parameters) {
      BAIL_IF(!cursor.require(Code::Type::AddressOp));
      name = cursor.require(Code::Type::Addressable);
      BAIL_IF(!name || !cursor.require(Code::Type::Define));
      const auto text = cursor.get_text(name);
      for (const auto field : output.get_view()) {
        if (field.get_data() == text) {
          cursor.create_token_error(name, "Duplicate parameter name."_view);
          return False;
        }
      }
    }

    const Token type_token = cursor.require(Code::Type::Type);
    BAIL_IF(!type_token);
    const auto& reference = arena.construct<Source::Policies::Reference>(
        types, arena.proxy(cursor.get_text(type_token)),
        cursor.get_anchor(type_token));
    const auto& field = arena.construct<Model::Execution::Fields::Value>(
        Abstract::provide(reference));
    output.insert(authored(
        arena, cursor, Abstract::provide(field), name,
        cursor.get_anchor(
            Span(opening, type_token), name ? name : type_token)));
    if (!cursor.matches(Code::Type::PackingOp)) {
      break;
    }

    cursor.consume();
  }

  return cursor.require(Code::Type::BracketEnd).is_valid();
}

static auto returned(
    Memory::Allocator::Arena& arena,
    Cursor& cursor,
    Abstract types,
    Core::View::Vector<Abstract> parameters) -> Core::Option<Abstract> {
  const Token token = cursor.current();
  if (cursor.matches(Code::Type::Addressable)) {
    const auto name = cursor.get_text();
    cursor.consume();
    for (const auto field : parameters) {
      // These fields were authored by this parser. Their labels are source
      // names, rather than an interpretation of arbitrary Abstract data.
      if (field.get_data() == name) {
        const auto& value =
            arena.construct<Model::Execution::Values::Parameter>(field);
        return authored(
            arena, cursor, Abstract::provide(value), Token(),
            cursor.get_anchor(token));
      }
    }

    cursor.create_token_error(
        token, "Return name does not identify a parameter."_view);
    return {};
  }

  if (cursor.matches(Code::Type::Numeric)) {
    Core::Reader::Textual number(cursor.get_text());
    const U64 value = number.read_unsigned();
    if (!number.is_valid() || value > U32(-1)) {
      cursor.create_token_error(token, "Constant exceeds U32 storage."_view);
      return {};
    }

    cursor.consume();
    // This literal's domain comes from the dialect's unsigned spelling. The
    // result declaration can request a different conversion, but must not
    // retroactively label four payload bytes as another source Type.
    const auto anchor = cursor.get_anchor(token);
    const auto& type =
        arena.construct<Source::Policies::Reference>(types, "U32"_view, anchor);
    const auto& literal =
        arena.construct<Model::Execution::Values::Literal<U32>>(
            Abstract::provide(type), U32(value));
    return authored(arena, cursor, Abstract::provide(literal), Token(), anchor);
  }

  cursor.create_token_error("Expected a parameter name or U32 constant."_view);
  return {};
}

// Parentheses group Library's positional flow, but do not create a stored
// tuple. Each leaf remains its own producer in the Return's ordered Layout.
// Empty groups therefore contribute no value and nesting preserves leaf order.
static auto returned_values(
    Memory::Allocator::Arena& arena,
    Cursor& cursor,
    Abstract types,
    Core::View::Vector<Abstract> parameters,
    Memory::Managed::Vector<Abstract>& output) -> Bool {
  if (!cursor.matches(Code::Type::PackingStart)) {
    auto value = returned(arena, cursor, types, parameters);
    BAIL_IF(!value);
    output.insert(*value);
    return True;
  }

  cursor.consume();
  while (!cursor.matches(Code::Type::PackingEnd)) {
    BAIL_IF(!returned_values(arena, cursor, types, parameters, output));
    if (cursor.matches(Code::Type::PackingEnd)) {
      break;
    }

    BAIL_IF(!cursor.require(
        Code::Type::PackingOp,
        "Return values require a comma or closing parenthesis."_view));
  }

  cursor.consume();
  return True;
}

// These facts describe the syntax just consumed. They are not retained as a
// completion state on the model. The terminal independently interprets the
// emitted sequence through its capabilities.
struct ParsedBlock {
  Abstract subject;
  Token closing;
  Bool returns;
};

static auto block(
    Memory::Allocator::Arena& arena,
    Cursor& cursor,
    Abstract types,
    Core::View::Vector<Abstract> parameters,
    Count result_count) -> Core::Option<ParsedBlock> {
  const Bool single = cursor.matches(Code::Type::Define);
  const Token opening =
      single ? cursor.consume() : cursor.require(Code::Type::ScopeStart);
  BAIL_IF(!opening);
  Memory::Managed::Vector<Abstract> statements(arena);
  Bool returns = False;
  Token closing;
  while (single || !cursor.matches(Code::Type::ScopeEnd)) {
    if (returns) {
      cursor.create_token_error(
          "Statement follows an unconditional return."_view);
      return {};
    }

    if (cursor.matches(Code::Type::ScopeStart)) {
      auto nested = block(arena, cursor, types, parameters, result_count);
      BAIL_IF(!nested);
      statements.insert(nested->subject);
      closing = nested->closing;
      returns = nested->returns;
    } else {
      const Token start = cursor.require(Code::Type::Return);
      BAIL_IF(!start);
      Memory::Managed::Vector<Abstract> values(arena);
      if (!cursor.matches(Code::Type::EndStatement)) {
        BAIL_IF(!returned_values(arena, cursor, types, parameters, values));
      }
      closing = cursor.require(Code::Type::EndStatement);
      BAIL_IF(!closing);
      if (values.get_size() != result_count) {
        cursor.create_expression_error(
            Span(start, closing),
            "Return flow does not cover the declared results."_view);
        return {};
      }
      const auto& flow = arena.construct<Model::Execution::Layouts::Sequence>(
          values.get_view());
      const auto& returned =
          arena.construct<Model::Execution::Statements::Return>(
              flow.get_interface());
      statements.insert(authored(
          arena, cursor, Abstract::provide(returned), Token(),
          cursor.get_anchor(Span(start, closing), start)));
      returns = True;
    }

    if (single) {
      break;
    }
  }

  if (!single) {
    closing = cursor.require(Code::Type::ScopeEnd);
    BAIL_IF(!closing);
  }
  const auto& sequence = arena.construct<Model::Execution::Statements::Block>(
      statements.get_view());
  return ParsedBlock(
      authored(
          arena, cursor, Abstract::provide(sequence), Token(),
          cursor.get_anchor(Span(opening, closing), opening)),
      closing, returns);
}

auto Dialect::Library::Function::interpret(
    Memory::Allocator::Arena& arena,
    Cursor& cursor,
    Abstract types,
    System::Uuid operation) -> Core::Option<Function&> {
  const Token opening = cursor.current();
  if (!cursor.is_one_of({{Code::Type::Public, Code::Type::Private}})) {
    cursor.create_token_error(
        "Function requires public or private visibility."_view);
    return {};
  }

  cursor.consume();
  const Token name = cursor.require(Code::Type::Addressable);
  BAIL_IF(!name || !cursor.require(Code::Type::Define));
  const Token qualifier = cursor.require(Code::Type::Func);
  BAIL_IF(!qualifier || !cursor.require(Code::Type::Assign));

  // Source syntax owns parameter names and evidence. The model receives only
  // those admitted fields and values, with their policy edges intact.
  Memory::Managed::Vector<Abstract> parameters(arena);
  Memory::Managed::Vector<Abstract> results(arena);
  BAIL_IF(!fields(arena, cursor, types, True, parameters));
  BAIL_IF(!cursor.require(Code::Type::CallOp));
  BAIL_IF(!fields(arena, cursor, types, False, results));
  const Token body_opening = cursor.current();
  auto body =
      block(arena, cursor, types, parameters.get_view(), results.get_size());
  BAIL_IF(!body);
  if (!body->returns && !results.is_empty()) {
    cursor.create_expression_error(
        Span(body_opening, body->closing),
        "Function body does not return its declared results."_view);
    return {};
  }

  const auto& input = arena.construct<Model::Execution::Layouts::Sequence>(
      parameters.get_view());
  const auto& output =
      arena.construct<Model::Execution::Layouts::Sequence>(results.get_view());
  const auto body_view = body->subject;
  const auto& model = arena.construct<Model::Execution::Functions::Function>(
      operation, input.get_interface(), output.get_interface(), body_view);
  const auto evidence = cursor.get_anchor(Span(opening, body->closing), name);
  auto& function = arena.construct<Function>(
      Tetrodotoxin::Source::Policies::Authored(
          Abstract::provide(model), evidence,
          arena.proxy(cursor.get_text(name))),
      parameters.get_view(), body_view);
  return function;
}

auto Dialect::Library::Function::resolve_concept(Core::View::Bytes route) const
    -> Abstract {
  if (route == "body"_view) {
    return body;
  }

  for (const auto field : parameters) {
    if (field.get_data() == route) {
      return field;
    }
  }

  return Answers::None::get_none();
}

auto Dialect::Library::Function::visit_concepts(Abstract::Visitor visitor) const
    -> void {
  for (const auto field : parameters) {
    visitor(field.get_data(), field);
  }

  visitor("body"_view, body);
}
