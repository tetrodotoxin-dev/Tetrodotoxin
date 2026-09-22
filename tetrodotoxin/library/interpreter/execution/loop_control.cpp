// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/execution/loop_control.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Execution::LoopControl::parse(
    Cursor& cursor,
    const Language::Flow::Block& lexical_context)
    -> Option<Language::Flow::LoopControl&> {
  Token opening = cursor.current();
  Language::Flow::LoopControl::Kind kind;
  if (cursor.matches(Code::Type::Break)) {
    kind = Language::Flow::LoopControl::Kind::Break;
  } else if (cursor.matches(Code::Type::Continue)) {
    kind = Language::Flow::LoopControl::Kind::Continue;
  } else {
    cursor.create_token_error(
        "Library loop control requires `break` or `continue`."_view);
    return {};
  }
  cursor.consume();
  auto target = lexical_context.get_enclosing_loop();
  if (!target) {
    cursor.create_token_error(
        opening, "Library loop control requires one enclosing loop."_view);
    return {};
  }
  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Library loop control requires one terminating `;`."_view);
  BAIL_IF(!closing);
  return Language::Flow::LoopControl::create_authored(
      cursor.get_arena(), kind, *target,
      Anchor::create(opening, Span(opening, closing)));
}
