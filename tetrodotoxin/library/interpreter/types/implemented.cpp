// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/types/implemented.hpp"

#include "tetrodotoxin/library/interpreter/type_reference.hpp"
#include "tetrodotoxin/library/interpreter/types/composite.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Types::Implemented::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition)
    -> Option<Parsed<Language::Types::Implemented>> {
  if (definition.get_authored().get_name().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Library implementation definitions require a Type shaped name."_view);
    return {};
  }

  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_authored().get_visibility(),
        "Library implementations accept only `public` or `private` visibility."_view);
    return {};
  }

  if (!definition.get_authored().get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_authored().get_modifiers().get_data()[0],
        "Library implementations do not accept evaluation modifiers."_view);
    return {};
  }

  Token kind_token = cursor.require(
      Code::Type::Implementation,
      "Library implementation definitions require the `implementation` qualifier."_view);
  BAIL_IF(!kind_token);
  auto requirement =
      Interpreter::TypeReference::parse(definition.get_host(), cursor);
  BAIL_IF(!requirement);
  auto& implemented = Language::Types::Implemented::create_authored(
      cursor.get_arena(), definition, *requirement);
  BAIL_IF(!implemented.bind_authored_requirement(cursor));
  ParseState state =
      Composite::parse_body(cursor, implemented, definition, kind_token);
  return Parsed<Language::Types::Implemented>(implemented, state);
}
