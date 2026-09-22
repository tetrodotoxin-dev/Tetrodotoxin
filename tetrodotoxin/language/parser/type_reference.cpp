// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/parser/type_reference.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto Language::Parser::TypeReference::parse(Cursor& cursor)
    -> Option<Tetrodotoxin::Language::TypeReference> {
  Token first = cursor.require(
      Code::Type::Type, "Type reference requires one Type name."_view);
  BAIL_IF(!first);

  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    cursor.consume();
    last = cursor.require(
        Code::Type::Type, "Type route requires a Type after `::`."_view);
    BAIL_IF(!last);
  }

  if (cursor.matches(Code::Type::BracketStart)) {
    cursor.create_token_error(
        cursor.current(),
        "Generic arguments require a formula owned by the consuming language."_view,
        "Use a completed named Type in this interface route."_view);
    return {};
  }

  Count start = first.get_offset();
  Count end = Count(last.get_offset()) + Count(last.get_size());
  return Tetrodotoxin::Language::TypeReference::create(
      cursor.get_source_text().slice(start, end - start),
      Anchor::create(first, Span(first, last)));
}
