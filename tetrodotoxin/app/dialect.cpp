// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/dialect.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/app/interpreter/program.hpp"
#include "tetrodotoxin/app/interpreter/runtime.hpp"
#include "tetrodotoxin/app/interpreter/scene.hpp"
#include "tetrodotoxin/app/language/monograph.hpp"
#include "tetrodotoxin/app/language/program.hpp"
#include "tetrodotoxin/app/language/runtime.hpp"
#include "tetrodotoxin/app/language/scene.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto App::Dialect::interpret(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation,
    const Anchor&,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  Option<App::Language::Runtime&> runtime;
  Option<App::Language::Program&> program;
  Option<App::Language::Scene&> scene;
  Bool failed = False;

  while (!cursor.matches(Code::Type::Terminal)) {
    const Tetrodotoxin::Source::Documentation& declaration_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    View::Bytes declaration = cursor.get_text();
    if (cursor.matches(Code::Type::Addressable) &&
        declaration == "runtime"_view) {
      if (runtime) {
        cursor.create_token_error(
            cursor.current(),
            "App accepts exactly one runtime declaration."_view);
        cursor.recover_to_statement();
        failed = True;
        continue;
      }

      auto parsed = App::Interpreter::Runtime::parse(
          cursor, declaration_documentation, context);
      if (!parsed) {
        cursor.recover_to_statement();
        failed = True;
        continue;
      }
      runtime = *parsed;
      continue;
    }

    if (cursor.matches(Code::Type::Addressable) &&
        declaration == "lifecycle"_view) {
      if (program || scene) {
        cursor.create_token_error(
            cursor.current(),
            "App accepts exactly one lifecycle declaration."_view);
        cursor.recover_to_statement();
        failed = True;
        continue;
      }

      if (App::Interpreter::Scene::is_next(cursor)) {
        auto parsed =
            App::Interpreter::Scene::parse(cursor, declaration_documentation);
        if (parsed) {
          scene = *parsed;
          continue;
        }
      } else {
        auto parsed =
            App::Interpreter::Program::parse(cursor, declaration_documentation);
        if (parsed) {
          program = *parsed;
          continue;
        }
      }
      cursor.recover_to_statement();
      failed = True;
      continue;
    }

    cursor.create_token_error(
        cursor.current(),
        "App accepts only `runtime` and `lifecycle` declarations."_view);
    cursor.recover_to_statement();
    failed = True;
  }

  if (!runtime) {
    cursor.create_error("App requires one runtime declaration."_view);
    failed = True;
  }
  if (!program && !scene) {
    cursor.create_error("App requires one lifecycle declaration."_view);
    failed = True;
  }
  if (failed) {
    return {};
  }

  return program ? Option<Tetrodotoxin::Language::Monograph&>(
                       App::Language::Monograph::create_program(
                           cursor.get_arena(), *this, documentation, context,
                           *runtime, *program))
                 : Option<Tetrodotoxin::Language::Monograph&>(
                       App::Language::Monograph::create_scene(
                           cursor.get_arena(), *this, documentation, context,
                           *runtime, *scene));
}
