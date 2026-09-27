// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/app/language/program.hpp"
#include "tetrodotoxin/app/language/runtime.hpp"
#include "tetrodotoxin/app/language/scene.hpp"
#include "tetrodotoxin/language/monograph.hpp"

namespace Tetrodotoxin::App::Language {

// Monograph owns one completed application policy while its Runtime and
// Program identities remain in the same source transaction Arena.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  static auto create_program(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Abstract& language,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Abstract& context,
      Runtime& runtime,
      Program& program) -> Monograph&;

  static auto create_scene(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Abstract& language,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Abstract& context,
      Runtime& runtime,
      Scene& scene) -> Monograph&;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;
  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool override;

  TTX_NAME("App"_view);

  constexpr auto get_runtime() const -> const Runtime& { return runtime; }
  constexpr auto get_program() const
      -> Perimortem::Core::Option<const Program&> {
    return program.visit(
        []() -> Perimortem::Core::Option<const Program&> { return {}; },
        [](const Program& selected)
            -> Perimortem::Core::Option<const Program&> { return selected; });
  }
  constexpr auto get_scene() const -> Perimortem::Core::Option<const Scene&> {
    return scene.visit(
        []() -> Perimortem::Core::Option<const Scene&> { return {}; },
        [](const Scene& selected) -> Perimortem::Core::Option<const Scene&> {
          return selected;
        });
  }

 private:
  constexpr Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Abstract& language,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Abstract& context,
      Runtime& runtime,
      Perimortem::Core::Option<Program&> program,
      Perimortem::Core::Option<Scene&> scene)
      : Tetrodotoxin::Language::Monograph(
            arena,
            language,
            documentation,
            context),
        runtime(runtime),
        program(program),
        scene(scene) {}

  Runtime& runtime;
  Perimortem::Core::Option<Program&> program;
  Perimortem::Core::Option<Scene&> scene;
};

}  // namespace Tetrodotoxin::App::Language
