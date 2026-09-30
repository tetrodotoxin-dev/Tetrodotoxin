// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/capabilities/export.hpp"

#include <stdio.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/dialects/build/workspace.hpp"
#include "tetrodotoxin/dialects/source/stream.hpp"
#include "ttx/semantic/negotiation/library.h"

using namespace Tetrodotoxin::Dialects;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;

// This terminal binds the real Workspace and Source contracts supplied by
// Build. Two separately loaded copies write different files, so the process
// test can observe whether every Export saw the complete set of retained roots.
// Malformed input is refused before writing to make terminal admission visible.
class SourceExporter {
 public:
  explicit SourceExporter(Perimortem::Core::View::Bytes filename)
      : filename(filename) {}

  auto get_data() const -> Perimortem::Core::View::Bytes {
    return Perimortem::Core::View::Bytes();
  }

  auto supports(Perimortem::System::Uuid id) const -> Binding::Status {
    return id == Capabilities::Export::contract_id ? Binding::Status::Satisfied
                                                   : Binding::Status::Unknown;
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage target) const -> Binding::Status {
    if (id == Capabilities::Export::contract_id) {
      return Binding::provide<Capabilities::Export>(
          Capabilities::Export::provide(*this).get_abi(), target);
    }

    return Binding::Status::Unknown;
  }

  auto expose(Abstract graph) const -> Binding::Status {
    return graph.bind<Build::Workspace>().visit(
        [&](Build::Workspace workspace) {
          Perimortem::Memory::Dynamic::Bytes content;
          bool valid = true;
          auto observe = [&](Perimortem::Core::View::Bytes name,
                             Abstract root) {
            root.bind<Source::Stream>().visit(
                [&](Source::Stream stream) {
                  const auto tokens = stream.get_tokens();
                  if (!tokens.is_valid() ||
                      stream.get_lexicon() !=
                          Source::Stream::lexical_contract) {
                    valid = false;
                    return;
                  }
                  for (Count i = 0; i < tokens.get_size(); ++i) {
                    if (tokens[i].get_code() == Source::Code::Type::Unknown) {
                      valid = false;
                      return;
                    }
                  }
                  if (!valid) {
                    valid = false;
                    return;
                  }

                  Perimortem::Core::Static::Vector<char, 32> count;
                  const int size = snprintf(
                      count.get_data(), count.get_size(), "\t%llu\n",
                      static_cast<unsigned long long>(tokens.get_size() - 1));
                  content.concat(name);
                  content.concat(
                      Perimortem::Core::View::Bytes(
                          reinterpret_cast<const U8*>(count.get_data()),
                          Count(size)));
                },
                [&](Binding::Failure) { valid = false; });
          };

          workspace.visit_concepts(Abstract::Visitor(observe));
          if (!valid) {
            return Binding::Status::Rejected;
          }

          auto output = Perimortem::System::File::Root::open(
              workspace.get_output_directory());
          if (!output) {
            return Binding::Status::Rejected;
          }

          const auto written = output->write(content.get_view(), filename);
          return written ? Binding::Status::Satisfied
                         : Binding::Status::Rejected;
        },
        [](Binding::Failure failure) {
          return static_cast<Binding::Status>(failure);
        });
  }

 private:
  Perimortem::Core::View::Bytes filename;
};

extern "C" ttx_binding_status ttx_query(
    ttx_semantic_query,
    ttx_query_receiver receiver) {
  if (!receiver.receive) {
    return TTX_BINDING_REJECTED;
  }

  const SourceExporter exporter(
      Perimortem::Core::NullTerminated::to_view(
          CATALOG_SECOND ? "second.txt" : "first.txt"));
  return receiver.receive(
      receiver.source, Abstract::provide(exporter).get_query());
}
