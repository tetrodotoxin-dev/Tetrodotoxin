// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/interpreter/emission.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/library/interpreter/pack.hpp"
#include "tetrodotoxin/scene/language/emission.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto Scene::Interpreter::Emission::matches(const Cursor& cursor) const -> Bool {
  return cursor.matches(Code::Type::Emit);
}

auto Scene::Interpreter::Emission::parse(
    Cursor& cursor,
    Library::Language::Flow::Block& block,
    Library::Language::Model::Callable&,
    const Library::Language::Model::Type&,
    const Tetrodotoxin::Source::Documentation& documentation) const
    -> Option<Library::Language::Statement> {
  Token opening = cursor.consume();
  Token signal = cursor.require(
      Code::Type::Addressable, "Scene emission requires one Signal name."_view);
  BAIL_IF(!signal);

  Option<Library::Language::Model::Pack&> payload;
  if (cursor.matches(Code::Type::PackingStart)) {
    payload = Library::Interpreter::Pack::parse(block, cursor, True);
    BAIL_IF(!payload);
  }
  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Scene emission requires one terminating `;`."_view);
  BAIL_IF(!closing);

  auto& emission = Scene::Language::Emission::create_authored(
      cursor.get_arena(), monograph,
      signal.caculate_text(cursor.get_source_text()), signal, payload,
      Anchor::create(opening, Span(opening, closing)));
  BAIL_IF(!monograph.retain_emission(emission));
  return Library::Language::Statement::create(
      emission, documentation, emission.get_anchor(),
      [](Scene::Language::Emission& selected, Cursor& operation_cursor,
         Library::Language::Flow::Scope& scope) {
        return selected.link(operation_cursor, scope);
      },
      [](Scene::Language::Emission& selected, Cursor& operation_cursor) {
        selected.finalize(operation_cursor);
      });
}
