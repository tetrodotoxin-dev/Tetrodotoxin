// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/types/interface.hpp"

#include "tetrodotoxin/library/interpreter/types/composite.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Types::Interface::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition)
    -> Option<Parsed<Language::Types::Interface>> {
  if (definition.get_authored().get_name().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Library Interface definitions require a Type shaped name."_view);
    return {};
  }

  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_authored().get_visibility(),
        "Library Interfaces accept only `public` or `private` visibility."_view);
    return {};
  }

  if (!definition.get_authored().get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_authored().get_modifiers().get_data()[0],
        "Library Interfaces do not accept evaluation modifiers."_view);
    return {};
  }

  Token kind_token = cursor.require(
      Code::Type::Interface,
      "Library Interface definitions require the `interface` qualifier."_view);
  BAIL_IF(!kind_token);
  Language::Types::Interface& interface =
      Language::Types::Interface::create_authored(
          cursor.get_arena(), definition);
  ParseState state =
      Composite::parse_body(cursor, interface, definition, kind_token);
  return Parsed<Language::Types::Interface>(interface, state);
}
