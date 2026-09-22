// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/interpreter/source.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/render/interpreter/declaration.hpp"

using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Render;

auto Interpreter::Source::parse(Language::Monograph& monograph, Cursor& cursor)
    -> void {
  while (!cursor.matches(Code::Type::Terminal)) {
    const Tetrodotoxin::Source::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    if (!Declaration::parse(monograph, cursor, documentation)) {
      cursor.recover_to_statement();
    }
  }
}
