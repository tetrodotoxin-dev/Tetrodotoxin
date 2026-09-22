// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/types/object.hpp"

#include "tetrodotoxin/library/interpreter/types/composite.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Types::Object::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition)
    -> Option<Parsed<Language::Types::Object>> {
  if (definition.get_authored().get_name().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Library Object definitions require a Type shaped name."_view);
    return {};
  }
  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_authored().get_visibility(),
        "Library Objects accept only `public` or `private` visibility."_view);
    return {};
  }
  if (!definition.get_authored().get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_authored().get_modifiers().get_data()[0],
        "Library Objects do not accept evaluation modifiers."_view);
    return {};
  }

  Token kind_token = cursor.require(
      Code::Type::Object,
      "Library Object definitions require the `object` qualifier."_view);
  BAIL_IF(!kind_token);
  Language::Types::Object& object =
      Language::Types::Object::create_authored(cursor.get_arena(), definition);
  ParseState state =
      Composite::parse_body(cursor, object, definition, kind_token);
  return Parsed<Language::Types::Object>(object, state);
}
