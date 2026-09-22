// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/parser/layout.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto Language::Parser::Layout::require_shape(
    Cursor& cursor,
    Core::Option<Bool>& selected,
    Bool named) -> Bool {
  if (!selected) {
    selected = named;
    return True;
  }
  if (*selected == named) {
    return True;
  }
  cursor.create_token_error(
      "Positional and named entries cannot share one Layout."_view);
  return False;
}

auto Language::Parser::Layout::retain_name(
    Cursor& cursor,
    Token token,
    Memory::Managed::Vector<Core::View::Bytes>& names) -> Bool {
  Core::View::Bytes name = token.caculate_text(cursor.get_source_text());
  if (names.get_view().contains(
          [&](Core::View::Bytes retained) { return retained == name; })) {
    cursor.create_token_error(token, "Duplicate name in one Layout."_view);
    return False;
  }

  // The concrete model retains this source view when it needs delayed linking
  // or presentation because the transaction Arena keeps the source alive.
  names.insert(name);
  return True;
}
