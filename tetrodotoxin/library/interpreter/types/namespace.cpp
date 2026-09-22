// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/types/namespace.hpp"

#include "tetrodotoxin/library/interpreter/types/composite.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Types::Namespace::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition)
    -> Option<Parsed<Language::Types::Namespace>> {
  if (definition.get_authored().get_name().get_code() != Code::Type::Type ||
      definition.get_visibility() ==
          Tetrodotoxin::Language::Visibility::Exposed ||
      !definition.get_authored().get_modifiers().is_empty()) {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Library Namespace definitions require a public or private Type-shaped name without modifiers."_view);
    return {};
  }

  Token kind = cursor.require(
      Code::Type::Namespace,
      "Library Namespace definitions require the `namespace` qualifier."_view);
  BAIL_IF(!kind);
  auto& selected = Language::Types::Namespace::create_authored(
      cursor.get_arena(), definition);
  ParseState state = Composite::parse_body(cursor, selected, definition, kind);
  return Parsed<Language::Types::Namespace>(selected, state);
}
