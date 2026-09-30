// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stdio.h>
#include <string.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/dialects/build/input.hpp"
#include "tetrodotoxin/dialects/build/workspace.hpp"
#include "ttx/concept/capabilities/import.hpp"
#include "ttx/semantic/negotiation/library.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Dialects;
using namespace Ttx::Concept;
using namespace Ttx::Data::Form;
using namespace Ttx::Semantic::Negotiation;

static auto bytes(const char* text) -> perimortem_view_bytes {
  const auto view = NullTerminated::to_view(text);
  return {view.get_data(), view.get_size()};
}

static auto report(perimortem_view_bytes message) -> void {
  fwrite(message.data, 1, message.size, stderr);
  fputc('\n', stderr);
}

auto main(int argc, char** argv) -> int {
  Perimortem::Memory::Dynamic::Vector<tetrodotoxin_build_module> modules;
  Perimortem::Memory::Dynamic::Vector<tetrodotoxin_build_source> sources;
  perimortem_view_bytes output{};
  bool valid = argc >= 2;
  for (int index = 2; index < argc && valid;) {
    if (strcmp(argv[index], "--module") == 0 && argc - index >= 3) {
      modules.insert({bytes(argv[index + 1]), bytes(argv[index + 2])});
      index += 3;
    } else if (strcmp(argv[index], "--source") == 0 && argc - index >= 4) {
      sources.insert(
          {bytes(argv[index + 1]), bytes(argv[index + 2]),
           bytes(argv[index + 3])});
      index += 4;
    } else if (
        strcmp(argv[index], "--output") == 0 && argc - index >= 2 &&
        !output.data) {
      output = bytes(argv[index + 1]);
      index += 2;
    } else {
      valid = false;
    }
  }
  if (!valid || !output.data || !modules.get_size() || !sources.get_size()) {
    fputs(
        "Usage: puffer <build-module> --module <name> <path> ... "
        "--source <name> <path> <importer> ... --output <directory>\n",
        stderr);
    return 2;
  }

  // Puffer owns the process request and its diagnostic presentation. Build owns
  // plugin discovery, file acquisition, graph lifetime and terminal dispatch.
  const tetrodotoxin_build_input input{
    modules.get_view().get_data(),
    modules.get_size(),
    sources.get_view().get_data(),
    sources.get_size(),
    output,
    nullptr,
    [](void*, perimortem_view_bytes message) { report(message); }};
  Perimortem::Memory::Allocator::Arena errors;
  return Library::open(NullTerminated::to_view(argv[1]), errors)
      .visit(
          [&](Library library) {
            bool completed = false;
            auto receive = [&](Query query) {
              return query.bind<Capabilities::Import>().visit(
                  [&](Capabilities::Import importer) {
                    auto observe = [&](Abstract root) {
                      completed = root.bind<Build::Workspace>().visit(
                          [](Build::Workspace) { return true; },
                          [](Binding::Failure) { return false; });
                    };
                    return importer.visit(
                        &input,
                        Compiled<Native<tetrodotoxin_build_input>::reference>::
                            get_representation(),
                        observe);
                  },
                  [](Binding::Failure failure) {
                    return static_cast<Binding::Status>(failure);
                  });
            };
            const auto status = library.visit(Query(), Receiver(receive));
            if (status != Binding::Status::Satisfied || !completed) {
              report(bytes("Build did not complete"));
              return 1;
            }
            puts("Build complete");
            return 0;
          },
          [](View::Bytes error) {
            report({error.get_data(), error.get_size()});
            return 2;
          });
}
