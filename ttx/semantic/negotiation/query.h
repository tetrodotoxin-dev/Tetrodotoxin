// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_SEMANTIC_NEGOTIATION_QUERY_H
#define TTX_SEMANTIC_NEGOTIATION_QUERY_H

#include "perimortem/system/uuid.h"

#include "ttx/semantic/negotiation/binding.h"

// Two systems need an agreed entry point before either can ask the other for
// an interface. A module can return this Query from its entry function, or a
// native owner can lend it directly. That enclosing agreement supplies the
// bind thunk and its lifetime, so calling bind does not first require a Flow.
// Systems can therefore begin cooperating without constructing a TTX graph.
// Once bootstrapped it is possible to use `bind` to acquire future `bind`
// contracts from a Query provider (thinking binding an Abstract Plugin that
// provides a `bind` interface for managing other surfaces).
//
// Each call asks for one UUID and supplies the concrete form the caller can
// consume. Success populates that admitted Storage with the promised API.
// The canonical descriptor builds off of the TTX::Data protocol and includes
// callable signatures along with their calling conventions, so this exchange
// checks how to call the supplied API as well as how to store it. This is why
// alternative `bind` providers remains useful but having a Query provider in
// the systems native calling conventions makes bootstrapping vastly easier than
// trial and erroring the entire surface. Marker contracts use empty Storage
// because their answer consists only of a status. Binding's status and
// borrowing rules are described in ttx/semantic/negotiation/binding.h.
//
// Actually accessing `source` inside of TTX is undefined behavior as far as
// Tetrodotoxin is concerned. It can _technically_ be safe but making any
// assumptions about the lifetime or wire format of the source is a quick way to
// get into trouble since sources allow for dynamic substitution under TTX's
// semantic simulacra principle.
//
// The enclosing owner keeps the supplying state and implementation code alive
// through negotiation and every use of the returned bindings. Copying this
// Query borrows that agreement without acquiring another lifetime.
typedef struct ttx_semantic_query {
  const void* source;
  ttx_binding_status (*bind)(
      const void* source,
      perimortem_uuid contract,
      ttx_storage requested);
} ttx_semantic_query;

#endif
