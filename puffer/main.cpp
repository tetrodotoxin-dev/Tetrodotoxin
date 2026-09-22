// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <sys/stat.h>

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "puffer/lsp/methods.hpp"
#include "tetrodotoxin/build/dialect.hpp"
#include "tetrodotoxin/environment/toolchain.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source::Lexical;

// It's easy to mistake Puffer as a "compiler driver", but really it's a
// "toolchain driver". This actually lets Puffer be sneakily simple by design
// since it uses the larger Tetrodotoxin infrastructure to build up a dynamic
// layered toolchain.
//
// Puffer only includes:
// - Tetrodotoxin::Enviornment to construct the base toolchain
// - Tetrodotoxin::Build as the starting Dialect
//
// It alternatively supports a legacy LSP for TTX which will eventually be
// ported to Tetrodotoxin proper.
//
// With those two systems the rest of the Dialect engine is built. This means by
// default Puffer can't actually read most common TTX sources. `Build` more or
// less is the required entry point so any toolchain extensions must use `Build`
// to bootstrap using the Dialect system.
//
// You can read up on the Build Dialect for how to customize puffer. By default
// Tetrodotoxin does actually include _a_ "compiler driver" toolchain using LLVM
// if you want to look at building applications out of pure TTX.
//
// Alternatively you can write your own frontend driver and preload your set of
// hardcoded Dialects if you know exactly what Monographs you want to process.
S32 main(S32 argc, char** argv) {
  Diagnostics::Log::set_sink(Diagnostics::Log::plain_sink);

  // Puffer needs to do argument forwarding to the actual toolchains so it only
  // extracts the first positional argument. If there are no positional args
  // then it renders help. An actual "-help" flag would be propagated to the
  // toolchain.
  if (argc <= 1 || argv[1][0] == '-') {
    Diagnostics::Log::info(
        "Puffer requires a single file as it's first positional argument."_view);
    return 0;
  }

  const auto source = NullTerminated::to_view(argv[1]);
  struct stat status = {};
  if (stat(argv[1], &status) == 0 && S_ISSOCK(status.st_mode)) {
    // TODO: Move pipe dispatch into an LSP Dialect when the LSP system is
    // introduced.
    return Puffer::Lsp::run(source);
  }

  // Build interprets the meaning of the remaining arguments so we preserve
  // their order and spelling but strip the source argument that's consumed
  // to point at the first file to load.
  Allocator::Arena arguments_arena;
  Managed::Vector<View::Bytes> arguments(arguments_arena);
  for (S32 index = 2; index < argc; index++) {
    arguments.insert(NullTerminated::to_view(argv[index]));
  }

  // Registration borrows the actual Dialect so we need to make sure it lives
  // longer than the toolchain it's installed into. The toolchain itself doesn't
  // perform any clean up so we can use a stack reference for this oneshot.
  Tetrodotoxin::Build::Dialect build(arguments.get_view());
  Tetrodotoxin::Environment::Toolchain toolchain;
  if (!toolchain.install(build)) {
    return 1;
  }

  // Run the toolchain and pass down an error accumulator.
  Tetrodotoxin::Source::Lexical::Errors errors;
  auto processed = toolchain.process(source, errors);

  // Render out any error messages that were accumulated from processing.
  if (errors.get_size() > 0) {
    Allocator::Arena arena;
    for (Count index = 0; index < errors.get_size(); index++) {
      Diagnostics::Log::error(errors.render_message(arena, index));
    }
  }

  return processed && errors.is_empty() ? 0 : 1;
}
