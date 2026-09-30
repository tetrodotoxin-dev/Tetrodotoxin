// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialects/build/workspace.hpp"

#include <cstdlib>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "tetrodotoxin/dialects/build/input.hpp"
#include "tetrodotoxin/dialects/source/stream.hpp"
#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/concept/capabilities/import.hpp"
#include "ttx/semantic/negotiation/library.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Dialects;
using namespace Ttx::Concept;
using namespace Ttx::Data::Form;
using namespace Ttx::Semantic::Negotiation;

static auto bytes(const char* value) -> perimortem_view_bytes {
  const auto text = NullTerminated::to_view(value);
  return perimortem_view_bytes(text.get_data(), text.get_size());
}

// Retaining the Build result must keep its imported roots and supplying code
// alive after Import returns. Querying Source through that workspace and then
// releasing it exercises the lifetime that a synchronous Puffer run cannot
// establish by checking terminal output alone.
auto main(int argc, char** argv) -> int {
  const char* output = std::getenv("TEST_TMPDIR");
  if (argc != 5 || !output) {
    return 2;
  }

  const Static::Vector<tetrodotoxin_build_module, 2> modules = {{
    tetrodotoxin_build_module(bytes("source"), bytes(argv[2])),
    tetrodotoxin_build_module(bytes("terminal"), bytes(argv[3])),
  }};
  const Static::Vector<tetrodotoxin_build_source, 1> sources = {{
    tetrodotoxin_build_source(bytes("main"), bytes(argv[4]), bytes("source")),
  }};
  const tetrodotoxin_build_input input(
      modules.get_data(), modules.get_size(), sources.get_data(),
      sources.get_size(), bytes(output), nullptr, nullptr);
  Perimortem::Memory::Allocator::Arena errors;
  return Library::open(NullTerminated::to_view(argv[1]), errors)
      .visit(
          [&](Library library) {
            Option<Policies::Borrowed> retained;
            auto receive = [&](Query query) {
              return query.bind<Capabilities::Import>().visit(
                  [&](Capabilities::Import importer) {
                    auto observe = [&](Abstract root) {
                      root.bind<Capabilities::Borrow>().visit(
                          [&](Capabilities::Borrow borrow) {
                            borrow.borrow().visit(
                                [&](Policies::Borrowed answer) {
                                  retained = answer;
                                },
                                [](Binding::Failure) {});
                          },
                          [](Binding::Failure) {});
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
            if (!retained) {
              return 3;
            }

            const bool valid = retained->bind<Build::Workspace>().visit(
                [&](Build::Workspace workspace) {
                  const auto source = workspace.resolve_concept("main"_view);
                  return source.bind<Source::Stream>().visit(
                      [&](Source::Stream stream) {
                        return stream.get_tokens().get_size() == 8 &&
                               workspace.get_output_directory() ==
                                   NullTerminated::to_view(output) &&
                               workspace.supports<Policies::Borrowed>() ==
                                   Binding::Status::Satisfied;
                      },
                      [](Binding::Failure) { return false; });
                },
                [](Binding::Failure) { return false; });
            retained->release();
            return valid && status == Binding::Status::Satisfied ? 0 : 4;
          },
          [](View::Bytes) { return 5; });
}
