// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/dialect.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/shader/archive/reader.hpp"
#include "tetrodotoxin/shader/archive/writer.hpp"
#include "tetrodotoxin/shader/interpreter/source.hpp"
#include "tetrodotoxin/shader/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto Shader::Dialect::interpret(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  auto& child = Library::Language::Monograph::create_authored(
      cursor.get_arena(), documentation, source_anchor, library, context);
  auto& monograph = Shader::Language::Monograph::create(
      cursor.get_arena(), *this, documentation, context, child);
  Shader::Interpreter::Source::parse(monograph, cursor);
  return monograph;
}

auto Shader::Dialect::encode(const Abstract& monograph) const
    -> Option<Perimortem::Memory::Dynamic::Bytes> {
  auto shader = monograph.select<Shader::Language::Monograph>();
  BAIL_IF(!shader);
  return Shader::Archive::Writer::encode(*shader);
}

auto Shader::Dialect::decode(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes payload,
    Abstract& context) -> Option<Abstract&> {
  return Shader::Archive::Reader::restore(arena, payload, *this, library, context);
}
