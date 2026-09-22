// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/spirv/module/program.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/terminal/spirv/assembler/spir_v.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Terminal::Spirv;

auto Module::Program::report_failure(Core::View::Bytes message) const -> void {
  Tetrodotoxin::Source::Lexical::Errors::Report report(
      request.get_errors(), request.get_source_path(),
      request.get_source_text(), request.get_program().get_anchor());
  report << message;
  report.get_hint()
      << "Keep the Shader within the selected SPIR V target surface."_view;
}

auto Module::Program::compile() -> Utility::Result<Products, Failure> {
  if (request.get_target() != Target::Vulkan1_0 ||
      !request.get_monograph().is_finalized()) {
    return Failure::ToolchainFailed;
  }
  Bool selected = False;
  for (const Tetrodotoxin::Source::Reference<Shader::Language::Program>& program :
       request.get_monograph().get_programs()) {
    selected |= &program.get() == &request.get_program();
  }
  if (!selected) {
    return Failure::ToolchainFailed;
  }

  Count error_count = request.get_errors().get_size();
  // Collection completes target admission before the first word is emitted.
  // Unsupported meaning stays on the authored diagnostic path and no partial
  // module reaches the caller.
  Bool prepared = interface.prepare(request.get_program());
  for (Interface::Stage* stage : interface.get_stages()) {
    prepared &= body.prepare(*stage);
  }
  for (const Tetrodotoxin::Source::Reference<Shader::Language::Bridge>& bridge :
       request.get_monograph().get_bridges()) {
    auto gpu = bridge.get().get_gpu_type();
    auto library_type =
        gpu ? gpu->select<Library::Language::Model::Type>()
            : Core::Option<const Library::Language::Model::Type&>();
    prepared &= library_type && types.collect(*library_type);
  }
  if (!prepared) {
    if (request.get_errors().get_size() == error_count) {
      report_failure(
          "The selected Shader uses meaning this SPIR V target cannot represent."_view);
    }
    return Failure::SourceRejected;
  }

  Memory::Dynamic::Bytes words;
  Assembler::SpirV assembler(words);
  // SPIR V sections have a required order even though ids may refer forward to
  // later declarations. Each owner emits only its physical section and shares
  // the one module id allocator.
  assembler.begin_module(0);
  assembler.capability(Assembler::SpirV::Capability::Shader);
  if (types.requires_float64()) {
    assembler.capability(Assembler::SpirV::Capability::Float64);
  }
  assembler.memory_model(
      Assembler::SpirV::AddressingModel::Logical,
      Assembler::SpirV::MemoryModel::GLSL450);
  interface.emit_entry_points(assembler);
  interface.emit_debug(assembler);
  Bool emitted = interface.emit_annotations(assembler);
  if (!emitted) {
    report_failure(
        "The selected Pipeline interface has no valid SPIR V decoration."_view);
  }
  if (emitted && !types.emit(assembler)) {
    report_failure(
        "The selected Library Type graph has no complete SPIR V representation."_view);
    emitted = False;
  }
  if (emitted && !interface.emit_types(assembler)) {
    report_failure(
        "The selected Shader push interface has no complete SPIR V representation."_view);
    emitted = False;
  }
  if (emitted && !constants.emit(assembler)) {
    report_failure(
        "One folded Library value has no complete SPIR V representation."_view);
    emitted = False;
  }
  if (emitted && !interface.emit_globals(assembler)) {
    report_failure(
        "The selected Pipeline interface could not emit its global variables."_view);
    emitted = False;
  }
  for (Interface::Stage* stage : interface.get_stages()) {
    if (emitted) {
      Count stage_error_count = request.get_errors().get_size();
      if (!body.emit(*stage, assembler)) {
        if (request.get_errors().get_size() == stage_error_count) {
          Tetrodotoxin::Source::Lexical::Errors::Report report(
              request.get_errors(), request.get_source_path(),
              request.get_source_text(), stage->function.get().get_anchor());
          report << "Shader Stage `"_view << stage->function.get().get_name()
                 << "` could not emit a complete SPIR V body."_view;
          report.get_hint()
              << "Use executable Library meaning admitted by the SPIR V target."_view;
        }
        emitted = False;
      }
    }
  }
  if (emitted && !assembler.patch_bound(ids.get_bound())) {
    return Failure::ToolchainFailed;
  }
  if (emitted && !Assembler::SpirV::is_valid_module(words)) {
    return Failure::ToolchainFailed;
  }
  if (!emitted) {
    if (request.get_errors().get_size() == error_count) {
      report_failure(
          "The selected Shader could not produce a complete SPIR V module."_view);
    }
    return Failure::SourceRejected;
  }

  return Products(arena.proxy(words.get_view()));
}
