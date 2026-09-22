// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/language/monograph.hpp"

#include "tetrodotoxin/source/documentation.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::App;

auto Language::Monograph::create_program(
    Allocator::Arena& arena,
    const Abstract& language,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& context,
    Runtime& runtime,
    Program& program) -> Monograph& {
  return arena.construct_from<Monograph>([&]() {
    return Monograph(
        arena, language, documentation, context, runtime, program, {});
  });
}

auto Language::Monograph::create_scene(
    Allocator::Arena& arena,
    const Abstract& language,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& context,
    Runtime& runtime,
    Scene& scene) -> Monograph& {
  return arena.construct_from<Monograph>([&]() {
    return Monograph(
        arena, language, documentation, context, runtime, {}, scene);
  });
}

auto Language::Monograph::link(Cursor& cursor) -> Bool {
  return program ? program->link(cursor, *this) : scene->link(cursor, *this);
}

auto Language::Monograph::finalize(Cursor&) -> Bool {
  return program ? Bool(program->get_entry())
                 : Bool(scene->get_initial_scene());
}
