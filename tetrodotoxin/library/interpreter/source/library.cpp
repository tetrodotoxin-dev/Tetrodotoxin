// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/source/library.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/library/interpreter/source/foreign.hpp"
#include "tetrodotoxin/library/interpreter/source/import.hpp"

using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Source::Library::parse(
    Language::Types::Source& source,
    Cursor& cursor) -> void {
  while (!cursor.matches(Code::Type::Terminal)) {
    const Tetrodotoxin::Source::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);

    if (cursor.matches(Code::Type::Using)) {
      auto import = Import::parse(cursor, documentation);
      if (import) {
        source.retain_import_route(*import);
      }
      continue;
    }

    if (Foreign::is_next(cursor)) {
      Foreign::parse(source.get_foreign(), cursor, documentation);
      continue;
    }

    auto definition = Tetrodotoxin::Language::Definition::parse(
        cursor, documentation, source);
    if (!definition) {
      cursor.recover_to_statement();
      continue;
    }

    auto member = Interpreter::Member::parse(cursor, *definition);
    if (!member) {
      cursor.recover_to_statement();
      continue;
    }
    source.retain_authored_definition(
        member->get_semantic(), *definition, member->get_category(), cursor);
    if (member->needs_recovery()) {
      cursor.recover_to_statement();
    }
  }
}
