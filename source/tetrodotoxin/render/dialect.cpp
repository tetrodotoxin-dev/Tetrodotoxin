// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/dialect.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/render/archive/reader.hpp"
#include "tetrodotoxin/render/archive/writer.hpp"
#include "tetrodotoxin/render/interpreter/source.hpp"
#include "tetrodotoxin/render/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto Render::Dialect::interpret(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation,
    const Anchor&,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  auto& monograph = Render::Language::Monograph::create(
      cursor.get_arena(), *this, documentation, context);
  Render::Interpreter::Source::parse(monograph, cursor);
  return monograph;
}

auto Render::Dialect::encode(const Abstract& monograph) const
    -> Option<Dynamic::Bytes> {
  auto render = monograph.select<Render::Language::Monograph>();
  BAIL_IF(!render);
  return Render::Archive::Writer::encode(*render);
}

auto Render::Dialect::decode(
    Allocator::Arena& arena,
    View::Bytes payload,
    Abstract& context) -> Option<Abstract&> {
  return Render::Archive::Reader::restore(arena, payload, *this, context);
}
