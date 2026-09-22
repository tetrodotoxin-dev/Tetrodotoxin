// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/interpreter/route.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto App::Interpreter::Route::parse(Cursor& cursor) -> Option<Language::Route> {
  Token first = cursor.require(
      Code::Type::Type, "App route requires one Type shaped name."_view);
  BAIL_IF(!first);
  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    cursor.consume();
    last = cursor.require(
        Code::Type::Type, "App route requires a Type name after `::`."_view);
    BAIL_IF(!last);
  }
  Count start = first.get_offset();
  Count end = Count(last.get_offset()) + Count(last.get_size());
  return Language::Route::create_authored(
      cursor.get_source_text().slice(start, end - start),
      Anchor::create(first, Span(first, last)));
}
