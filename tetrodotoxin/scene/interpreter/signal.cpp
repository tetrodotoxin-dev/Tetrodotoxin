// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/interpreter/signal.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/library/interpreter/type_reference.hpp"
#include "tetrodotoxin/scene/language/signal.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto Scene::Interpreter::Signal::is_next(const Cursor& cursor) -> Bool {
  return cursor.matches(Code::Type::Addressable) &&
         cursor.current().caculate_text(cursor.get_source_text()) ==
             "signal"_view;
}

auto Scene::Interpreter::Signal::parse(
    Language::Monograph& monograph,
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation) -> Bool {
  BAIL_IF(!is_next(cursor));
  Token opening = cursor.consume();
  Token name = cursor.require(
      Code::Type::Addressable, "Scene Signal requires one name."_view);
  BAIL_IF(!name);

  Option<Library::Language::TypeReference> payload;
  if (cursor.matches(Code::Type::Define)) {
    cursor.consume();
    payload = Library::Interpreter::TypeReference::parse(
        monograph.get_instance(), cursor);
    BAIL_IF(!payload);
  }

  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Scene Signal declaration requires one terminating `;`."_view);
  BAIL_IF(!closing);

  auto& signal = Scene::Language::Signal::create_authored(
      cursor.get_arena(), documentation,
      name.caculate_text(cursor.get_source_text()), name, payload,
      Anchor::create(name, Span(opening, closing)));
  cursor.get_associations().create(
      Anchor::create(opening, Span(opening)), signal);
  return monograph.retain_signal(signal, cursor);
}
