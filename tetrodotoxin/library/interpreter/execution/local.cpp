// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/execution/local.hpp"

#include "tetrodotoxin/library/interpreter/expressions/initializer.hpp"
#include "tetrodotoxin/library/interpreter/pack.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Execution::Local::parse(
    Cursor& cursor,
    Language::Flow::Block& host) -> Core::Option<Language::Flow::Local&> {
  Token evaluation = cursor.current();
  Language::Writability writability = Language::Writability::Full;
  switch (evaluation.get_code().get_type()) {
  case Code::Type::State:
    cursor.consume();
    break;
  case Code::Type::Const:
    cursor.consume();
    writability = Language::Writability::Constant;
    break;
  default:
    cursor.create_token_error(
        "Library Local declarations require `state` or `const`."_view);
    return {};
  }

  Token name = cursor.require(
      Code::Type::Addressable,
      "Library Local declarations require one addressable name."_view);
  BAIL_IF(!name);
  BAIL_IF(!cursor.require(
      Code::Type::Define,
      "Library Local declarations require `:` after their name."_view));

  Core::Option<Language::TypeReference> type_reference;
  Core::Option<Language::Model::Pack&> initializer;
  if (cursor.matches(Code::Type::Assign)) {
    cursor.consume();
    if (Interpreter::Expressions::Initializer::is_next(cursor)) {
      auto object_initializer =
          Interpreter::Expressions::Initializer::parse(host, cursor);
      BAIL_IF(!object_initializer);
      initializer = *object_initializer;
    } else {
      initializer = Interpreter::Pack::parse(host, cursor);
    }
    BAIL_IF(!initializer);
  } else {
    auto declared_type = Interpreter::TypeReference::parse(host, cursor);
    BAIL_IF(!declared_type);
    type_reference = *declared_type;
    if (cursor.matches(Code::Type::Assign)) {
      cursor.consume();
      if (Interpreter::Expressions::Initializer::is_next(cursor)) {
        auto object_initializer =
            Interpreter::Expressions::Initializer::parse(host, cursor);
        BAIL_IF(!object_initializer);
        initializer = *object_initializer;
      } else {
        initializer = Interpreter::Pack::parse(host, cursor);
        BAIL_IF(!initializer);
      }
    }
  }

  if (writability == Language::Writability::Constant && !initializer) {
    cursor.create_token_error(
        "Library const Locals require one compile time initializer."_view);
    return {};
  }
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Local declarations require one terminating `;`."_view);
  BAIL_IF(!terminator);
  Anchor anchor = Anchor::create(name, Span(evaluation, terminator));
  Language::Flow::Local& local = Language::Flow::Local::create_authored(
      cursor.get_arena(), host, name,
      name.caculate_text(cursor.get_source_text()), writability, type_reference,
      initializer, anchor);
  cursor.get_associations().create(anchor, local);
  return local;
}
