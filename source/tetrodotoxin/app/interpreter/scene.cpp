// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/interpreter/scene.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/app/interpreter/route.hpp"
#include "tetrodotoxin/app/language/transition.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

static auto require_text(
    Cursor& cursor,
    Code::Type code,
    View::Bytes text,
    View::Bytes message) -> Token {
  if (!cursor.matches(code) || cursor.get_text() != text) {
    cursor.create_token_error(cursor.current(), message);
    return {};
  }
  return cursor.consume();
}

struct ParsedAction {
  App::Language::Transition::Action action;
  Token token;
};

static auto parse_action(Cursor& cursor) -> Option<ParsedAction> {
  Token token = cursor.require(
      Code::Type::Addressable,
      "App Scene transition requires replace, push, pop, or exit."_view);
  BAIL_IF(!token);
  View::Bytes name = token.caculate_text(cursor.get_source_text());
  if (name == "replace"_view) {
    return ParsedAction{App::Language::Transition::Action::Replace, token};
  }
  if (name == "push"_view) {
    return ParsedAction{App::Language::Transition::Action::Push, token};
  }
  if (name == "pop"_view) {
    return ParsedAction{App::Language::Transition::Action::Pop, token};
  }
  if (name == "exit"_view) {
    return ParsedAction{App::Language::Transition::Action::Exit, token};
  }
  cursor.create_token_error(
      token, "App Scene transition accepts replace, push, pop, or exit."_view);
  return {};
}

auto App::Interpreter::Scene::is_next(const Cursor& cursor) -> Bool {
  return cursor.matches(Code::Type::Addressable) &&
         cursor.get_text() == "lifecycle"_view &&
         cursor.peek(1).get_code() == Code::Type::Assign &&
         cursor.peek(2).get_code() == Code::Type::Type &&
         cursor.peek(2).caculate_text(cursor.get_source_text()) == "Scene"_view;
}

auto App::Interpreter::Scene::parse(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation) -> Option<Language::Scene&> {
  Token opening = require_text(
      cursor, Code::Type::Addressable, "lifecycle"_view,
      "App Scene lifecycle requires `lifecycle`."_view);
  BAIL_IF(!opening);
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "App lifecycle requires `=` before its Scene policy."_view));
  BAIL_IF(!require_text(
      cursor, Code::Type::Type, "Scene"_view,
      "App Scene lifecycle requires the `Scene` profile."_view));
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "App Scene lifecycle requires one `{}` body."_view));
  Token initial_token = require_text(
      cursor, Code::Type::Addressable, "initial"_view,
      "App Scene lifecycle begins with one `initial` Scene."_view);
  BAIL_IF(!initial_token);
  auto initial = App::Interpreter::Route::parse(cursor);
  BAIL_IF(!initial);
  BAIL_IF(!cursor.require(
      Code::Type::EndStatement,
      "App initial Scene requires one terminating `;`."_view));

  Managed::Vector<Reference<Language::Transition>> transitions(
      cursor.get_arena());
  while (!cursor.matches(Code::Type::ScopeEnd) &&
         !cursor.matches(Code::Type::Terminal)) {
    const Tetrodotoxin::Source::Documentation& transition_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    Token transition_opening = require_text(
        cursor, Code::Type::Addressable, "on"_view,
        "App Scene mapping requires `on`."_view);
    BAIL_IF(!transition_opening);
    auto source = App::Interpreter::Route::parse(cursor);
    BAIL_IF(!source);
    BAIL_IF(!cursor.require(
        Code::Type::AddressOp,
        "App Scene mapping requires `.` before its Signal."_view));
    Token signal = cursor.require(
        Code::Type::Addressable,
        "App Scene mapping requires one Signal name."_view);
    BAIL_IF(!signal);
    auto action = parse_action(cursor);
    BAIL_IF(!action);

    Option<Language::Route> destination;
    if (action->action == Language::Transition::Action::Replace ||
        action->action == Language::Transition::Action::Push) {
      destination = App::Interpreter::Route::parse(cursor);
      BAIL_IF(!destination);
    }
    Token closing = cursor.require(
        Code::Type::EndStatement,
        "App Scene mapping requires one terminating `;`."_view);
    BAIL_IF(!closing);

    View::Bytes signal_name = signal.caculate_text(cursor.get_source_text());
    for (const Reference<Language::Transition>& retained :
         transitions.get_view()) {
      if (retained.get().get_source_route().get_spelling() ==
              source->get_spelling() &&
          retained.get().get_signal_name() == signal_name) {
        cursor.create_token_error(
            signal, "App Scene lifecycle already maps this exact Signal."_view);
        return {};
      }
    }

    auto& transition = Language::Transition::create_authored(
        cursor.get_arena(), transition_documentation, *source, signal_name,
        Anchor::create(signal, Span(signal)), action->action, destination,
        Anchor::create(transition_opening, Span(transition_opening, closing)));
    cursor.get_associations().create(
        Anchor::create(transition_opening, Span(transition_opening)),
        transition);
    cursor.get_associations().create(
        Anchor::create(action->token, Span(action->token)), transition);
    transitions.insert(transition);
  }

  Token closing = cursor.require(
      Code::Type::ScopeEnd,
      "App Scene lifecycle requires one closing `}`."_view);
  BAIL_IF(!closing);
  auto& scene = Language::Scene::create_authored(
      cursor.get_arena(), documentation, *initial, transitions.get_view(),
      Anchor::create(opening, Span(opening, closing)));
  cursor.get_associations().create(
      Anchor::create(opening, Span(opening)), scene);
  cursor.get_associations().create(
      Anchor::create(initial_token, Span(initial_token)), scene);
  return scene;
}
