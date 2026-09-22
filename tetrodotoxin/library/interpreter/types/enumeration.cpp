// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/types/enumeration.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

static auto parse_case(Cursor& cursor, const Tetrodotoxin::Source::Documentation& documentation)
    -> Core::Option<Language::Types::Enumeration::Case> {
  Token name_token = cursor.require(
      Code::Type::Addressable,
      "Library Enumeration cases require an addressable name."_view);
  BAIL_IF(!name_token);
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Library Enumeration cases require an explicit `=` value."_view));

  Token value_opening = cursor.current();
  Token value_token;
  if (cursor.matches(Code::Type::SubOp)) {
    cursor.consume();
    value_token = cursor.require(
        Code::Type::Numeric,
        "A negative Enumeration case requires a decimal integer."_view);
  } else if (
      cursor.matches(Code::Type::Numeric) || cursor.matches(Code::Type::Hex)) {
    value_token = cursor.consume();
  } else {
    cursor.create_token_error(
        "Library Enumeration cases require a decimal or hexadecimal "
        "integer."_view);
    return {};
  }
  BAIL_IF(!value_token);

  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Enumeration cases require one terminating `;`."_view);
  BAIL_IF(!terminator);
  Span value_span(value_opening, value_token);
  Core::View::Bytes source = cursor.get_source_text();
  return Language::Types::Enumeration::Case{
    .name = name_token.caculate_text(source),
    .value = value_span.caculate_text(source),
    .documentation = documentation,
    .anchor = Anchor::create(name_token, Span(name_token, terminator)),
    .name_anchor = Anchor::create(Span(name_token)),
    .value_anchor = Anchor::create(value_token, value_span),
  };
}

auto Interpreter::Types::Enumeration::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition)
    -> Core::Option<Parsed<Language::Types::Enumeration>> {
  if (definition.get_authored().get_name().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Library Enumerations require an authored Type shaped name."_view);
    return {};
  }
  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_authored().get_visibility(),
        "Library Enumerations accept only `public` or `private` visibility."_view);
    return {};
  }
  if (!definition.get_authored().get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_authored().get_modifiers().get_data()[0],
        "Library Enumerations do not accept evaluation modifiers."_view);
    return {};
  }

  Token enumeration_token = cursor.require(
      Code::Type::Enum,
      "Library Enumeration declarations require `enum`."_view);
  BAIL_IF(!enumeration_token);
  BAIL_IF(!cursor.require(
      Code::Type::BracketStart,
      "Library Enumeration storage requires an opening `[`."_view));
  auto storage = TypeReference::parse(definition.get_host(), cursor);
  BAIL_IF(!storage);
  BAIL_IF(!cursor.require(
      Code::Type::BracketEnd,
      "Library Enumeration storage requires a closing `]`."_view));
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Library Enumeration bodies require an opening `{`."_view));

  Memory::Managed::Vector<Language::Types::Enumeration::Case> cases(
      cursor.get_arena());
  ParseState state = ParseState::Accepted;
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Library Enumeration body reached the end of source before "
          "`}`."_view);
      state = ParseState::Rejected;
      break;
    }

    const Tetrodotoxin::Source::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto parsed = parse_case(cursor, documentation);
    if (!parsed) {
      state = ParseState::Rejected;
      cursor.recover_to_scoped_statement();
      continue;
    }
    if (cases.get_view().contains([&](const auto& existing) {
          return existing.name == parsed->name;
        })) {
      cursor.create_expression_error(
          parsed->name_anchor,
          "Duplicate case name in one Library Enumeration."_view);
      state = ParseState::Rejected;
      continue;
    }
    cases.insert(*parsed);
  }

  if (cursor.matches(Code::Type::ScopeEnd)) {
    Token closing = cursor.consume();
    auto& authored = definition.get_authored();
    authored.set_anchor(Anchor::create(
        enumeration_token,
        Span(authored.get_anchor().get_span().get_start(), closing)));
  }

  auto& enumeration = Language::Types::Enumeration::create_authored(
      cursor.get_arena(), definition, *storage, cases.get_view());
  return Parsed<Language::Types::Enumeration>(enumeration, state);
}
