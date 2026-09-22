// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/declarations/alias.hpp"

#include "tetrodotoxin/library/interpreter/type_reference.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Declarations::Alias::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition)
    -> Option<Parsed<Language::Alias>> {
  if (definition.get_authored().get_name().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Library Alias definitions require a Type shaped name."_view);
    return {};
  }

  if (definition.get_visibility() >
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_authored().get_visibility(),
        "Library Aliases accept only `expose` or `private` visibility."_view);
    return {};
  }

  if (!definition.get_authored().get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_authored().get_modifiers().get_data()[0],
        "Library Aliases do not accept evaluation modifiers."_view);
    return {};
  }

  Token alias_token = cursor.require(
      Code::Type::Alias,
      "Library Alias definitions require the `alias` qualifier."_view);
  BAIL_IF(!alias_token);
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Library Alias qualifiers require `=` before their Type route."_view));

  auto target_reference =
      Interpreter::TypeReference::parse(definition.get_host(), cursor);
  if (!target_reference) {
    auto missing = Language::TypeReference::create(
        {}, Anchor::create(Span(definition.get_authored().get_name())), Token());
    auto& alias = Language::Alias::create_authored(
        cursor.get_arena(), definition, missing);
    return Parsed<Language::Alias>(alias, ParseState::Incomplete);
  }
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Alias definitions require one terminating `;`."_view);
  ParseState state = ParseState::Incomplete;
  if (terminator) {
    auto& authored = definition.get_authored();
    authored.set_anchor(Anchor::create(
        alias_token,
        Span(authored.get_anchor().get_span().get_start(), terminator)));
    state = ParseState::Accepted;
  }

  auto& alias = Language::Alias::create_authored(
      cursor.get_arena(), definition, *target_reference);
  return Parsed<Language::Alias>(alias, state);
}
