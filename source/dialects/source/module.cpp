// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialects/source/dialect.hpp"
#include "ttx/semantic/negotiation/library.h"

// The provider is available only inside the loader's synchronous observation.
// Source results request their own retention through Borrow, while the host
// keeps this module loaded until those acquisitions have all been released.
extern "C" ttx_binding_status ttx_query(
    ttx_semantic_query,
    ttx_query_receiver receiver) {
  if (!receiver.receive) {
    return TTX_BINDING_REJECTED;
  }
  return receiver.receive(
      receiver.source,
      Tetrodotoxin::Dialects::Source::Dialect::importer().get_query());
}
