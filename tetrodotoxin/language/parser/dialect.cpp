// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/parser/dialect.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/source/lexical/lexicon.hpp"
#include "tetrodotoxin/source/lexical/token.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Language;

auto Parser::Dialect::parse(Cursor& cursor) -> View::Bytes {
  BAIL_IF(!cursor.require(
      Code::Type::Dialect,
      "Source files are required to select a dialect using `dialect : Type;`."_view));

  BAIL_IF(!cursor.require(
      Code::Type::Define,
      "Expected `:` after the source Dialect declaration."_view));

  Token dialect_token = cursor.require(
      Code::Type::Type,
      "Expected a concrete source Dialect name such as `Package` or `Library`."_view);
  BAIL_IF(!dialect_token);

  View::Bytes dialect_name =
      dialect_token.caculate_text(cursor.get_source_text());
  BAIL_IF(!cursor.require(
      Code::Type::EndStatement,
      "Expected `;` after the source Dialect declaration."_view));

  return dialect_name;
}
