// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/access/swizzle.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Access::Swizzle::parse(
    const Abstract&,
    Cursor& cursor,
    Language::Model::Pack& receiver,
    Span receiver_span) -> Core::Option<Language::Expression&> {
  Token opening = cursor.consume();
  Memory::Managed::Vector<Token> tokens(cursor.get_arena());
  Memory::Managed::Vector<Core::View::Bytes> names(cursor.get_arena());
  while (!cursor.matches(Code::Type::BracketEnd)) {
    Token name = cursor.require(
        Code::Type::Addressable,
        "Swizzle requires an addressable name or one closing bracket."_view);
    BAIL_IF(!name);
    tokens.insert(name);
    names.insert(name.caculate_text(cursor.get_source_text()));
    if (!cursor.matches(Code::Type::PackingOp)) {
      break;
    }
    cursor.consume();
    if (cursor.matches(Code::Type::BracketEnd)) {
      break;
    }
  }

  Token closing = cursor.require(
      Code::Type::BracketEnd,
      "Swizzle requires one closing bracket after its selected names."_view);
  BAIL_IF(!closing);
  return Language::Access::Swizzle::create_authored(
      cursor.get_arena(), receiver, tokens.get_view(), names.get_view(),
      Anchor::create(opening, receiver_span, Span(closing)));
}
