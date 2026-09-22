// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/expressions/initializer.hpp"

#include "tetrodotoxin/library/interpreter/pack.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Expressions::Initializer::is_next(const Cursor& cursor)
    -> Bool {
  return cursor.matches(Code::Type::New);
}

auto Interpreter::Expressions::Initializer::parse(
    const Abstract& context,
    Cursor& cursor) -> Core::Option<Language::Expressions::Initializer&> {
  Memory::Allocator::Arena& domain = cursor.get_arena();
  BAIL_IF(!is_next(cursor));

  Token opening = cursor.consume();
  BAIL_IF(!cursor.require(
      Code::Type::BracketStart,
      "Library `new` requires `[` before its Object Type."_view));
  auto target_reference = TypeReference::parse(context, cursor);
  BAIL_IF(!target_reference);
  Token type_closing = cursor.require(
      Code::Type::BracketEnd,
      "Library `new` requires `]` after its Object Type."_view);
  BAIL_IF(!type_closing);

  Core::Option<Language::Model::Pack&> arguments;
  Token closing = type_closing;
  if (cursor.matches(Code::Type::PackingStart)) {
    Token argument_opening = cursor.current();
    Token argument_closing = cursor.peek(1);
    if (argument_closing.get_code().get_type() == Code::Type::PackingEnd) {
      cursor.create_expression_error(
          Span(argument_opening, argument_closing),
          "Initializer arguments cannot be empty."_view,
          "Omit the argument list to request the Type default."_view);
      return {};
    }
    auto parsed = Interpreter::Pack::parse(context, cursor, True);
    BAIL_IF(!parsed);
    arguments = *parsed;
    closing = cursor.peek(-1);
  } else {
    // Each omitted argument list owns its empty Pack because linking and
    // finalization belong to the surrounding source transaction.
    arguments = Language::Model::Pack::create_empty(domain);
  }

  return Language::Expressions::Initializer::create_authored(
      domain, *target_reference, *arguments,
      Anchor::create(opening, Span(opening, closing)));
}
