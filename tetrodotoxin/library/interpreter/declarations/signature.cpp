// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/declarations/signature.hpp"

#include "tetrodotoxin/library/interpreter/layout.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Declarations::Signature::parse(
    Cursor& cursor,
    const Abstract& host) -> Option<Language::Signature&> {
  auto parameters = Interpreter::Layout::parse_model(
      cursor, host, True);
  BAIL_IF(!parameters);
  BAIL_IF(!cursor.require(
      Code::Type::CallOp,
      "Library Function parameters require `->` before the result "
      "Layout."_view));
  auto results = Interpreter::Layout::parse_model(
      cursor, host, False);
  BAIL_IF(!results);
  return Language::Signature::create_authored(
      cursor.get_arena(), host, *parameters, *results);
}
