// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/capabilities/import.hpp"
#include "ttx/semantic/negotiation/library.h"

using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;

// The default Import returns None without retained access. This provider is
// deliberately valid at the TTX boundary, so its use as either a source
// importer or a Build module tests the consuming system's result requirements
// rather than the loader's response to a broken module.
class EmptyImporter {
 public:
  auto get_data() const -> Perimortem::Core::View::Bytes {
    return Perimortem::Core::View::Bytes();
  }

  auto supports(Perimortem::System::Uuid id) const -> Binding::Status {
    return id == Capabilities::Import::contract_id ? Binding::Status::Satisfied
                                                   : Binding::Status::Unknown;
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage target) const -> Binding::Status {
    if (id == Capabilities::Import::contract_id) {
      return Binding::provide<Capabilities::Import>(
          Capabilities::Import::provide(*this).get_abi(), target);
    }

    return Binding::Status::Unknown;
  }
};

extern "C" ttx_binding_status ttx_query(
    ttx_semantic_query,
    ttx_query_receiver receiver) {
  if (!receiver.receive) {
    return TTX_BINDING_REJECTED;
  }

  const EmptyImporter importer;
  return receiver.receive(
      receiver.source, Abstract::provide(importer).get_query());
}
