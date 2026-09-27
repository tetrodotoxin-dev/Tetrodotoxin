// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/access/call.hpp"

#include "tetrodotoxin/library/interpreter/pack.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Access::Call::parse(
    const Abstract& context,
    Cursor& cursor,
    Language::Model::Pack& receiver) -> Option<Language::Expression&> {
  Token operation = cursor.require(
      Code::Type::CallOp,
      "Library invocation requires `->` before its Callable name."_view);
  BAIL_IF(!operation);
  Token name_token = cursor.require(
      Code::Type::Addressable,
      "Library invocation requires one Callable name after `->`."_view);
  BAIL_IF(!name_token);
  Token arguments_opening = cursor.current();
  auto arguments = Interpreter::Pack::parse(context, cursor, True);
  BAIL_IF(!arguments);
  Token closing = cursor.peek(-1);
  auto receiver_anchor = receiver.get_anchor();
  if (!receiver_anchor) {
    cursor.create_expression_error(
        Anchor::create(name_token, Span(operation, closing)),
        "Library invocation requires an authored receiver Anchor."_view);
    return {};
  }

  return Language::Access::Call::create_authored(
      cursor.get_arena(), receiver, name_token,
      name_token.caculate_text(cursor.get_source_text()), *arguments,
      Anchor::create(
          name_token, receiver_anchor->get_span(),
          Span(arguments_opening, closing)));
}
