// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/interpreter/source.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/library/interpreter/source/foreign.hpp"
#include "tetrodotoxin/library/interpreter/source/import.hpp"
#include "tetrodotoxin/scene/interpreter/lifecycle.hpp"
#include "tetrodotoxin/scene/interpreter/signal.hpp"

using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto Scene::Interpreter::Source::parse(
    Language::Monograph& monograph,
    Cursor& cursor) -> void {
  auto& source = monograph.edit_library().get_source();
  auto& instance = monograph.edit_instance();
  while (!cursor.matches(Code::Type::Terminal)) {
    const Tetrodotoxin::Source::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);

    if (cursor.matches(Code::Type::Using)) {
      auto import =
          Library::Interpreter::Source::Import::parse(cursor, documentation);
      if (import) {
        source.retain_import_route(*import);
      }
      continue;
    }

    if (Library::Interpreter::Source::Foreign::is_next(cursor)) {
      Library::Interpreter::Source::Foreign::parse(
          source.get_foreign(), cursor, documentation);
      continue;
    }

    if (Scene::Interpreter::Signal::is_next(cursor)) {
      if (!Scene::Interpreter::Signal::parse(
              monograph, cursor, documentation)) {
        cursor.recover_to_statement();
      }
      continue;
    }

    if (Scene::Interpreter::Lifecycle::is_next(cursor)) {
      if (!Scene::Interpreter::Lifecycle::parse(
              monograph, cursor, documentation)) {
        cursor.recover_to_statement();
      }
      continue;
    }

    auto definition = Tetrodotoxin::Language::Definition::parse(
        cursor, documentation, instance);
    if (!definition) {
      cursor.recover_to_statement();
      continue;
    }

    auto member = Library::Interpreter::Member::parse(cursor, *definition);
    if (!member) {
      cursor.recover_to_statement();
      continue;
    }
    instance.retain_authored_definition(
        member->get_semantic(), *definition, member->get_category(), cursor);
    if (member->needs_recovery()) {
      cursor.recover_to_statement();
    }
  }
  instance.complete_body();
}
