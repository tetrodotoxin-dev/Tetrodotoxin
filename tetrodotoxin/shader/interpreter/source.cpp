// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/source.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/shader/interpreter/program.hpp"

using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Interpreter::Source::parse(
    Shader::Language::Monograph& monograph,
    Cursor& cursor) -> void {
  const Tetrodotoxin::Source::Documentation& documentation =
      Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  if (!Interpreter::Program::parse(monograph, cursor, documentation)) {
    cursor.recover_to_statement();
  }
  if (!cursor.matches(Code::Type::Terminal)) {
    cursor.create_token_error(
        "Shader sources declare exactly one `implements` relationship."_view);
  }
}
