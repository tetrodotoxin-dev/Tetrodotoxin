// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "consumer.h"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "tetrodotoxin/dialects/source/input.hpp"
#include "tetrodotoxin/dialects/source/stream.hpp"
#include "ttx/semantic/negotiation/library.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Data::Form;
using namespace Ttx::Semantic::Negotiation;

// This host loads Source without linking its implementation and provides
// independently compiled public representations. The C observer must retain,
// read and release a stream across that module boundary after both caller
// buffers have changed. The loader stays alive until the final release.
auto main(int argc, char** argv) -> int {
  if (argc != 2) {
    return 2;
  }

  Perimortem::Memory::Allocator::Arena errors;
  return Library::open(NullTerminated::to_view(argv[1]), errors)
      .visit(
          [](Library library) {
            Static::Vector<U8, 18> text;
            Static::Vector<U8, 12> path;
            const auto original_text = "alpha 42\r\n// note\n"_view;
            const auto original_path = "consumer.ttx"_view;
            struct SourceBytes {
              View::Bytes bytes;
              auto get_data() const -> View::Bytes { return bytes; }
            } source_bytes{View::Bytes(text.get_data(), text.get_size())};
            const tetrodotoxin_source_input input(
                perimortem_view_bytes(path.get_data(), path.get_size()),
                Ttx::Concept::Abstract::provide(source_bytes).get_abi());
            source_consumer consumer(
                &Compiled<Native<tetrodotoxin_source_input>::reference>::
                    get_representation(),
                &Binding::representation<
                    Tetrodotoxin::Dialects::Source::Stream>(),
                &Compiled<Native<tetrodotoxin_source_token>::reference>::
                    get_representation(),
                ttx_borrowed(), 0);
            auto receive = [&](Query query) {
              return static_cast<Binding::Status>(
                  source_consumer_open(&consumer, query, &input));
            };
            Data::copy(
                text.get_data(), original_text.get_data(), text.get_size());
            Data::copy(
                path.get_data(), original_path.get_data(), path.get_size());

            const auto status = library.visit(Query(), Receiver(receive));
            Data::set(text.get_data(), 'x', text.get_size());
            Data::set(path.get_data(), 'x', path.get_size());

            const int result = source_consumer_finish(&consumer);
            return status == Binding::Status::Satisfied ? result : 3;
          },
          [](View::Bytes) { return 4; });
}
