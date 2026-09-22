// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/declarations/function.hpp"

#include "tetrodotoxin/library/interpreter/declarations/signature.hpp"
#include "tetrodotoxin/library/interpreter/execution/block.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

static auto validate_function(
    Cursor& cursor,
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  if (definition.get_authored().get_name().get_code() != Code::Type::Addressable) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Library Functions require an authored addressable name."_view);
    return False;
  }

  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_authored().get_visibility(),
        "Library Functions accept only `public` or `private` visibility."_view);
    return False;
  }

  if (!definition.get_authored().get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_authored().get_modifiers().get_data()[0],
        "Library Functions do not accept evaluation modifiers."_view);
    return False;
  }

  // Attributes remain source facts until the consumer of one key assigns its
  // meaning. Function grammar therefore leaves extension policy open while it
  // validates only the declaration shape it owns.
  return True;
}

auto Interpreter::Declarations::Function::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition)
    -> Option<Parsed<Language::Function>> {
  if (!validate_function(cursor, definition) ||
      !definition.get_host().is<Language::Model::Type>()) {
    return {};
  }

  BAIL_IF(!cursor.require(
      Code::Type::Func,
      "Library Function definitions require the `func` qualifier."_view));
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Library Function qualifiers require `=` before their signature."_view));

  auto signature = Interpreter::Declarations::Signature::parse(
      cursor, definition.get_host());
  BAIL_IF(!signature);
  Language::Function& function = Language::Function::create_authored(
      cursor.get_arena(), definition, *signature);
  Count error_count = cursor.get_error_count();
  auto body = Interpreter::Execution::Block::parse(
      cursor, function, function, function.get_host());
  if (!body) {
    return Parsed<Language::Function>(function, ParseState::Incomplete);
  }

  auto& authored = definition.get_authored();
  authored.set_anchor(Anchor::create(
      authored.get_qualifier(),
      Span(authored.get_anchor().get_span().get_start(),
           body->get_anchor().get_span().get_end())));
  Bool accepted = function.complete_body(*body) &&
                  cursor.get_error_count() == error_count;
  return Parsed<Language::Function>(
      function, accepted ? ParseState::Accepted : ParseState::Rejected);
}
