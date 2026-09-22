// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/declarations/field.hpp"

#include "tetrodotoxin/library/interpreter/expressions/initializer.hpp"
#include "tetrodotoxin/library/interpreter/pack.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

static auto parse_writability(
    const Tetrodotoxin::Language::Definition& definition,
    Cursor& cursor) -> Option<Language::Writability> {
  auto modifiers = definition.get_authored().get_modifiers();
  if (modifiers.get_size() > 1) {
    cursor.create_token_error(
        modifiers.get_data()[1],
        "Library Fields accept at most one evaluation modifier."_view);
    return {};
  }

  Language::Writability writability = Language::Writability::Full;
  if (!modifiers.is_empty()) {
    switch (modifiers.get_data()[0].get_code().get_type()) {
    case Code::Type::State:
      writability = Language::Writability::Internal;
      break;
    case Code::Type::Const:
      writability = Language::Writability::Constant;
      break;
    default:
      cursor.create_token_error(
          modifiers.get_data()[0],
          "Library Fields accept only `state` or `const` evaluation."_view);
      return {};
    }
  }

  Tetrodotoxin::Language::Visibility visibility = definition.get_visibility();
  if (visibility == Tetrodotoxin::Language::Visibility::Exposed &&
      writability != Language::Writability::Internal) {
    cursor.create_token_error(
        definition.get_authored().get_visibility(),
        "Library `expose` Fields require the `state` evaluation policy."_view);
    return {};
  }
  return writability;
}

auto Interpreter::Declarations::Field::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition)
    -> Option<Parsed<Language::Field>> {
  if (!definition.get_host().is<Language::Model::Type>()) {
    return {};
  }

  auto writability = parse_writability(definition, cursor);
  BAIL_IF(!writability);
  if (definition.get_authored().get_name().get_code() != Code::Type::Addressable) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Library Fields require an addressable name."_view);
    return {};
  }

  Option<Language::TypeReference> type;
  Option<Language::Model::Pack&> initializer;
  auto retain = [&](ParseState state) -> Parsed<Language::Field> {
    auto& field = Language::Field::create_authored(
        cursor.get_arena(), definition, *writability, type, initializer);
    return Parsed<Language::Field>(field, state);
  };

  if (cursor.matches(Code::Type::Assign)) {
    cursor.consume();
    if (Interpreter::Expressions::Initializer::is_next(cursor)) {
      auto object_initializer = Interpreter::Expressions::Initializer::parse(
          definition.get_host(), cursor);
      if (!object_initializer) {
        return retain(ParseState::Incomplete);
      }
      initializer = *object_initializer;
    } else {
      initializer = Interpreter::Pack::parse(definition.get_host(), cursor);
    }
    if (!initializer) {
      return retain(ParseState::Incomplete);
    }
  } else {
    auto authored_type =
        Interpreter::TypeReference::parse(definition.get_host(), cursor);
    if (!authored_type) {
      return retain(ParseState::Incomplete);
    }
    type = *authored_type;

    if (cursor.matches(Code::Type::Assign)) {
      cursor.consume();
      if (Interpreter::Expressions::Initializer::is_next(cursor)) {
        auto object_initializer = Interpreter::Expressions::Initializer::parse(
            definition.get_host(), cursor);
        if (!object_initializer) {
          return retain(ParseState::Incomplete);
        }
        initializer = *object_initializer;
      } else {
        initializer = Interpreter::Pack::parse(definition.get_host(), cursor);
      }
      if (!initializer) {
        return retain(ParseState::Incomplete);
      }
    } else if (*writability == Language::Writability::Constant) {
      cursor.create_token_error(
          "Library const Fields require an initializer."_view);
      return retain(ParseState::Incomplete);
    }
  }

  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Fields require one terminating `;`."_view);
  if (!terminator) {
    return retain(ParseState::Incomplete);
  }

  auto& authored = definition.get_authored();
  authored.set_anchor(Anchor::create(
      authored.get_name(),
      Span(authored.get_anchor().get_span().get_start(), terminator)));
  return retain(ParseState::Accepted);
}
