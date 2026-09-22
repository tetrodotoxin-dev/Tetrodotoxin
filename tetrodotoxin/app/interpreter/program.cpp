// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/interpreter/program.hpp"

#include "tetrodotoxin/source/documentation.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::App;

static auto require_text(
    Cursor& cursor,
    Code::Type code,
    View::Bytes text,
    View::Bytes message) -> Token {
  if (!cursor.matches(code) || cursor.get_text() != text) {
    cursor.create_token_error(cursor.current(), message);
    return {};
  }

  return cursor.consume();
}

auto Interpreter::Program::parse(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation) -> Option<Language::Program&> {
  Token opening = require_text(
      cursor, Code::Type::Addressable, "lifecycle"_view,
      "App lifecycle declaration requires `lifecycle`."_view);
  BAIL_IF(!opening);
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "App lifecycle declaration requires `=` before its policy."_view));
  BAIL_IF(!require_text(
      cursor, Code::Type::Type, "Program"_view,
      "This App slice accepts only Program lifecycle."_view));
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart, "Program lifecycle requires a `{}` body."_view));
  BAIL_IF(!require_text(
      cursor, Code::Type::Addressable, "start"_view,
      "Program lifecycle requires one `start` entry."_view));

  Token first = cursor.require(
      Code::Type::Type,
      "Program entry requires one Package member Type route."_view);
  BAIL_IF(!first);
  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    cursor.consume();
    last = cursor.require(
        Code::Type::Type,
        "Program entry route requires a Type after `::`."_view);
    BAIL_IF(!last);
  }

  Count route_start = first.get_offset();
  Count route_end = Count(last.get_offset()) + Count(last.get_size());
  View::Bytes route =
      cursor.get_source_text().slice(route_start, route_end - route_start);
  BAIL_IF(!cursor.require(
      Code::Type::CallOp,
      "Program entry requires `->` before its Static Callable."_view));
  Token callable = cursor.require(
      Code::Type::Addressable,
      "Program entry requires one Static Callable name."_view);
  BAIL_IF(!callable);

  if (cursor.matches(Code::Type::PackingOp) ||
      cursor.matches(Code::Type::EndStatement)) {
    cursor.consume();
  } else {
    cursor.create_token_error(
        cursor.current(), "Program entry requires a trailing `,` or `;`."_view);
    return {};
  }

  Token closing = cursor.require(
      Code::Type::ScopeEnd,
      "Program lifecycle accepts exactly one `start` entry."_view);
  BAIL_IF(!closing);

  auto& program = Language::Program::create_authored(
      cursor.get_arena(), documentation, route,
      callable.caculate_text(cursor.get_source_text()),
      Anchor::create(opening, Span(opening, closing)),
      Anchor::create(callable, Span(first, callable)));
  cursor.get_associations().create(program.get_anchor(), program);
  return program;
}
