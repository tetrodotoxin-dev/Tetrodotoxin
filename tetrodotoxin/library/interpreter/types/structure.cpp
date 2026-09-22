// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/types/structure.hpp"

#include "tetrodotoxin/library/interpreter/types/composite.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Types::Structure::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition)
    -> Option<Parsed<Language::Types::Structure>> {
  if (definition.get_authored().get_name().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Library Structure definitions require a Type shaped name."_view);
    return {};
  }

  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_authored().get_visibility(),
        "Library Structures accept only `public` or `private` visibility."_view);
    return {};
  }

  if (!definition.get_authored().get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_authored().get_modifiers().get_data()[0],
        "Library Structures do not accept evaluation modifiers."_view);
    return {};
  }

  Token kind_token = cursor.require(
      Code::Type::Struct,
      "Library Structure definitions require the `struct` qualifier."_view);
  BAIL_IF(!kind_token);
  Language::Types::Structure& structure =
      Language::Types::Structure::create_authored(
          cursor.get_arena(), definition);
  ParseState state =
      Composite::parse_body(cursor, structure, definition, kind_token);
  return Parsed<Language::Types::Structure>(structure, state);
}
