// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_SEMANTIC_NEGOTIATION_BINDING_H
#define TTX_SEMANTIC_NEGOTIATION_BINDING_H

#include "ttx/data/form/storage.h"

// A UUID describes the questions an interface answers, but cannot establish
// how to call a foreign implementation. Binding therefore supplies an actual
// API record together with its canonical Representation. The requester lends
// admitted Storage describing the record it can consume. Agreement compares
// the descriptors before any bytes are transferred or functions are called.
//
// The record may contain an opaque receiver and any number of operations.
// Copying those values grants no access to the receiver's private layout:
// each provider's functions still interpret their own state. Keeping that
// state opaque lets substitutes change storage without changing their answers.
//
// This synchronous exchange borrows the destination only until return. The
// enclosing publication retains the supplied receivers and executable code
// through every use of its interfaces. An interface with independent ownership
// describes those obligations in its own contract because copying pointers
// cannot acquire a lifetime. Ordinary Flow can materialize a record when
// needed.
//
// Unsupported permits policy forwarding. Pending and Rejected stop at the
// encountered policy. Failure supplies no usable API, even if a materializing
// provider touched the destination. Callers publish the record only on success.
//
// TODO: We need to figure out if we want to support hot reloading. Right now
// it's a higher order promise negotiated at the Concept layer or even the full
// Tetrodotoxin toolchain layer given the behavior is so domain specific but we
// should consider the invalidation case.
typedef U8 ttx_binding_status;
#define TTX_BINDING_SATISFIED ((ttx_binding_status)0)
#define TTX_BINDING_UNSUPPORTED ((ttx_binding_status)1)
#define TTX_BINDING_PENDING ((ttx_binding_status)2)
#define TTX_BINDING_REJECTED ((ttx_binding_status)3)

// Ready records use the same byte agreement as any other data publication.
// The supplying record and the admitted destination both have the geometry
// their descriptors promise otherwise the binding is rejected. Mismatched
// descriptors leave the destination untouched.
PERIMORTEM_C ttx_binding_status ttx_binding_provide(
    const ttx_representation* representation,
    const void* api,
    ttx_storage requested);

PERIMORTEM_C ttx_binding_status ttx_binding_marker(ttx_storage requested);

#endif
