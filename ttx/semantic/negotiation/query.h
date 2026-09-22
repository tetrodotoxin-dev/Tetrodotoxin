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
// A caller sometimes needs to know what a subject promises without acquiring
// any operations. Supports answers that semantic question using only the UUID.
// For example, an unsigned policy can exclude negative values without exposing
// a callable interface. The answer transfers no bytes and requires no agreed
// Representation. Pending preserves missing evidence and Rejected preserves
// a policy's refusal, just as they do during binding.
//
// Bind asks the stronger question: can this provider supply the promised API
// in the concrete form the caller can consume? Success populates that admitted
// Storage. Supports may succeed while every requested binding is rejected,
// since a shared semantic promise does not repair an incompatible API format.
// A support answer therefore grants no permission to call or cast anything.
// Callers needing operations can bind directly without a preliminary probe.
//
// The canonical descriptor builds off of the TTX::Data protocol and includes
// callable signatures along with their calling conventions, so this exchange
// checks how to call the supplied API as well as how to store it. This is why
// alternative `bind` providers remains useful but having a Query provider in
// the systems native calling conventions makes bootstrapping vastly easier than
// trial and erroring the entire surface. Marker bindings still agree on an
// empty API, while supports avoids that representation exchange altogether.
// Binding's status and
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
  ttx_binding_status (*supports)(const void* source, perimortem_uuid contract);
} ttx_semantic_query;

#endif
