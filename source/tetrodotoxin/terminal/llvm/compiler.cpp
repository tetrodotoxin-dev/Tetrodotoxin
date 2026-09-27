// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/compiler.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "lld/Common/Driver.h"
#include "llvm-c/Core.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/raw_ostream.h"
#include "tetrodotoxin/terminal/llvm/lowering/graph.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/graphics.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/projections.hpp"
#include "tetrodotoxin/terminal/llvm/module/program.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;

auto Llvm::Compiler::compile(
    Memory::Allocator::Arena& arena,
    const Request& request) const -> Utility::Result<Products, Failure> {
  U32 major = 0;
  U32 minor = 0;
  U32 patch = 0;
  LLVMGetVersion(&major, &minor, &patch);
  if (major != LLVM_VERSION_MAJOR || minor != LLVM_VERSION_MINOR ||
      patch != LLVM_VERSION_PATCH) {
    Core::Diagnostics::Log::error(
        "The loaded LLVM runtime does not match the pinned SDK."_view);
    return Failure::ToolchainFailed;
  }

  Llvm::Module::Program program(
      arena, request.get_errors(), request.get_source_path(),
      request.get_source_text(), request.get_target(),
      request.get_debug_level(), request.get_unit(), request.get_interface());
  Bool initialized = program.initialize();
  if (!initialized) {
    return Failure::ToolchainFailed;
  }

  if (!Llvm::Lowering::Graph::lower(
          program, request.get_monograph(), request.get_excluded())) {
    return program.has_source_failure() ? Failure::SourceRejected
                                        : Failure::ToolchainFailed;
  }

  auto graphics = request.get_graphics();
  if (graphics && !Llvm::Lowering::Graphics::lower(program, *graphics)) {
    return Failure::ToolchainFailed;
  }

  if (!Llvm::Lowering::Projections::lower(
          program, request.get_interface().get_projections())) {
    return Failure::ToolchainFailed;
  }

  return program.compile();
}

LLD_HAS_DRIVER(elf)

// LLD owns target linking and reports through the same process as compilation.
// Argument strings remain alive until the driver has completed all input reads.
auto Llvm::Compiler::link(Core::View::Vector<Core::View::Bytes> arguments) const
    -> Bool {
  Memory::Allocator::Arena arena;
  Memory::Dynamic::Vector<const char*> native;
  for (const auto argument : arguments) {
    Memory::Managed::Bytes text(arena, argument);
    text.append(0);
    native.insert(reinterpret_cast<const char*>(text.get_view().get_data()));
  }
  const lld::DriverDef drivers[] = {{lld::Gnu, &lld::elf::link}};
  const auto result = lld::lldMain(
      {native.get_data(), native.get_size()}, llvm::outs(), llvm::errs(),
      drivers);
  return result.retCode == 0;
}
