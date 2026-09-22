// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/dialect.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/shader/language/contract.hpp"
#include "tetrodotoxin/shader/language/monograph.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness ShaderDialect = {
  .name = "Tetrodotoxin::Shader::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(ShaderDialect, workspace_contract_and_stage) {
  static constexpr View::Bytes library_source =
      "// CPU and portable values.\n"
      "dialect : Library;"_view;
  static constexpr View::Bytes render_source =
      "// Pipeline interface.\n"
      "dialect : Pipeline;\n"
      "public fragment : stage [.color : Cpu::R64] -> [.color : Cpu::R64];\n"
      "public texture : resource read Cpu::U64;"_view;
  static constexpr View::Bytes shader_source =
      "// GPU implementation.\n"
      "dialect : Shader;\n"
      "implements Formats;\n"
      "public noise : resource read Cpu::S64 -> Cpu::U64;\n"
      "public gain : uniform Cpu::R64 = 0.0;\n"
      "Shader fragment[.color : Cpu::R64] -> [.color : Cpu::R64] {\n"
      "  state copied : Cpu::R64 = color + color + parameters.gain;\n"
      "  return (.color = copied);\n"
      "}\n"
      "@direction(\"upload\") @marshal(\"copy\") @sync(\"submission\")\n"
      "public count : bridge Cpu::U64 -> U64;"_view;

  Library::Dialect library;
  Render::Dialect render;
  Shader::Dialect shader(library, render);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(render));
  ASSERT(toolchain.install(shader));
  Environment::Workspace workspace(toolchain);
  Errors render_errors;
  Errors shader_errors;

  ASSERT(workspace.interpret_source(
      render_errors, "Cpu"_view, "cpu.ttx"_view, library_source));
  ASSERT(workspace.interpret_source(
      render_errors, "Formats"_view, "formats.ttx"_view, render_source));
  auto interpreted = workspace.interpret_source(
      shader_errors, "Programs"_view, "shader.ttx"_view, shader_source);

  ASSERT(interpreted && interpreted->is<Shader::Language::Monograph>());
  const auto& monograph =
      static_cast<const Shader::Language::Monograph&>(*interpreted);
  ASSERT(monograph.get_layer(shader));
  EXPECT(&*monograph.get_layer(shader) == &monograph);
  ASSERT(monograph.get_layer(library));
  EXPECT(&*monograph.get_layer(library) == &monograph.get_library());
  EXPECT_NOT(monograph.get_layer(render));
  ASSERT_EQ(monograph.get_programs().get_size(), Count(1));
  ASSERT_EQ(monograph.get_bridges().get_size(), Count(1));
  const auto& program = monograph.get_programs().get_data()[0].get();
  EXPECT(program.get_contract());
  Shader::Language::Contract negotiator;
  EXPECT(negotiator.accepts(*program.get_contract(), program));
  EXPECT(program.resolve_concept("static"_view)
             .resolve_concept("fragment"_view)
             .resolve()
             .is<Library::Language::Function>());
  EXPECT(program.resolve_concept("static"_view)
             .resolve_concept("texture"_view)
             .resolve()
             .is<Library::Language::Field>());
  EXPECT_EQ(program.get_bindings().get_size(), Count(3));
  auto gpu_noise = program.resolve_concept("static"_view)
                       .resolve_concept("noise"_view)
                       .resolve()
                       .select<Library::Language::Field>();
  auto runtime_noise = program.get_instance()
                           .resolve_concept("instance"_view)
                           .resolve_concept("noise"_view)
                           .resolve()
                           .select<Library::Language::Field>();
  ASSERT(gpu_noise && runtime_noise);
  EXPECT_TEXT(gpu_noise->get_type().get_name(), "U64"_view);
  EXPECT_TEXT(runtime_noise->get_type().get_name(), "S64"_view);
  auto bindings = program.get_bindings();
  auto runtime_binding = bindings.get_data()[0].get_instance_field();
  ASSERT(runtime_binding);
  EXPECT(&*runtime_binding == &*runtime_noise);
  ASSERT_EQ(program.get_uniforms().get_size(), Count(1));
  EXPECT_EQ(program.get_parameters().get_layout().get_size(), Count(1));
  EXPECT(program.satisfies(*program.get_contract()));
  EXPECT(monograph.get_bridges().get_data()[0].get().get_cpu_type());
  EXPECT(monograph.get_bridges().get_data()[0].get().get_gpu_type());
  EXPECT(render_errors.is_empty());
  EXPECT(shader_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ShaderDialect, rejects_incomplete_contract) {
  static constexpr View::Bytes library_source =
      "// CPU and portable values.\n"
      "dialect : Library;"_view;
  static constexpr View::Bytes render_source =
      "// Required GPU surface.\n"
      "dialect : Pipeline;\n"
      "public fragment : stage [] -> [];\n"
      "public texture : resource read Cpu::U64;"_view;
  static constexpr View::Bytes shader_source =
      "// Missing required Stage body.\n"
      "dialect : Shader;\n"
      "implements Formats;\n"
      "public seed : uniform Cpu::R64 = 0.0;"_view;

  Library::Dialect library;
  Render::Dialect render;
  Shader::Dialect installed_shader(library, render);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(library));
  ASSERT(toolchain.install(render));
  ASSERT(toolchain.install(installed_shader));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  ASSERT(workspace.interpret_source(
      errors, "Cpu"_view, "cpu.ttx"_view, library_source));
  ASSERT(workspace.interpret_source(
      errors, "Formats"_view, "required.ttx"_view, render_source));
  auto interpreted = workspace.interpret_source(
      errors, "Programs"_view, "broken-shader.ttx"_view, shader_source);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
  auto retained = workspace.get_monograph("broken-shader.ttx"_view);
  EXPECT(retained && retained->is<Shader::Language::Monograph>());
}
