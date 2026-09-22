// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/execution/return.hpp"

#include "tetrodotoxin/library/interpreter/pack.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Execution::Return::parse(
    Cursor& cursor,
    const Abstract& context) -> Option<Language::Flow::Return&> {
  Token operation = cursor.require(
      Code::Type::Return,
      "Library return statements require the `return` keyword."_view);
  BAIL_IF(!operation);
  Language::Model::Pack* pack = nullptr;
  if (!cursor.matches(Code::Type::EndStatement)) {
    auto parsed = Interpreter::Pack::parse(context, cursor);
    BAIL_IF(!parsed);
    pack = &*parsed;
  } else {
    pack = &Language::Model::Pack::create_empty(cursor.get_arena());
  }
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library return statements require one terminating `;`."_view);
  BAIL_IF(!terminator);
  return Language::Flow::Return::create_authored(
      cursor.get_arena(), Anchor::create(operation, Span(operation, terminator)),
      *pack);
}
