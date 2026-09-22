// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/interpreter/lifecycle.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/interpreter/declarations/signature.hpp"
#include "tetrodotoxin/library/interpreter/execution/block.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/scene/interpreter/emission.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

static auto select_role(View::Bytes name)
    -> Option<Scene::Language::Lifecycle> {
  if (name == "prepare"_view) {
    return Scene::Language::Lifecycle::Prepare;
  }
  if (name == "pause"_view) {
    return Scene::Language::Lifecycle::Pause;
  }
  if (name == "resume"_view) {
    return Scene::Language::Lifecycle::Resume;
  }
  if (name == "update"_view) {
    return Scene::Language::Lifecycle::Update;
  }
  if (name == "release"_view) {
    return Scene::Language::Lifecycle::Release;
  }
  return {};
}

auto Scene::Interpreter::Lifecycle::is_next(const Cursor& cursor) -> Bool {
  return cursor.matches(Code::Type::Type) &&
         cursor.current().caculate_text(cursor.get_source_text()) ==
             "Scene"_view;
}

auto Scene::Interpreter::Lifecycle::parse(
    Language::Monograph& monograph,
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation) -> Bool {
  BAIL_IF(!is_next(cursor));
  Token qualifier = cursor.consume();
  Token name_token = cursor.require(
      Code::Type::Addressable, "Scene lifecycle requires one role name."_view);
  BAIL_IF(!name_token);
  View::Bytes name = name_token.caculate_text(cursor.get_source_text());
  auto role = select_role(name);
  if (!role) {
    cursor.create_token_error(
        name_token,
        "Scene lifecycle accepts prepare, pause, resume, update, or release."_view);
    return False;
  }

  auto signature = Library::Interpreter::Declarations::Signature::parse(
      cursor, monograph.get_instance());
  BAIL_IF(!signature);

  auto& definition = Tetrodotoxin::Language::Definition::create_authored_prefix(
      cursor, documentation, monograph.edit_instance(), {}, {},
      Tetrodotoxin::Language::Visibility::Public, {}, name, name_token,
      qualifier, Anchor::create(name_token, Span(qualifier, cursor.peek(-1))));
  auto& function = Library::Language::Function::create_authored(
      cursor.get_arena(), definition, *signature);

  BAIL_IF(!monograph.edit_instance().retain_authored_definition(
      function, definition,
      Library::Language::Types::Composite::Category::Callable, cursor));
  BAIL_IF(!monograph.retain_lifecycle(*role, function, cursor));

  Scene::Interpreter::Emission emission(monograph);
  auto body = Library::Interpreter::Execution::Block::parse(
      cursor, function, function, monograph.get_instance(), {}, emission);
  BAIL_IF(!body);

  auto& authored = definition.get_authored();
  authored.set_anchor(Anchor::create(
      qualifier, Span(authored.get_anchor().get_span().get_start(),
                      body->get_anchor().get_span().get_end())));
  return function.complete_body(*body);
}
