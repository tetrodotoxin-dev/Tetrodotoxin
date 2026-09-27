// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/source/import.hpp"

#include "tetrodotoxin/library/interpreter/type_reference.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Source::Import::parse(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation)
    -> Option<Language::Import> {
  Token opening = cursor.require(
      Code::Type::Using, "Library Imports require `using`."_view);
  if (!opening) {
    cursor.recover_to_statement();
    return {};
  }

  auto type_reference = TypeReference::parse_route(cursor);
  if (!type_reference) {
    cursor.recover_to_statement();
    return {};
  }

  // The complete terminator keeps malformed trailing syntax out of the model
  // while the Cursor retains the diagnostic and recovery location.
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Imports require one terminating `;`."_view);
  if (!terminator) {
    cursor.recover_to_statement();
    return {};
  }
  return Language::Import(
      documentation, *type_reference, Span(opening, terminator));
}
