// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_SEMANTIC_FLOWS_COPY_H
#define TTX_SEMANTIC_FLOWS_COPY_H

#include "ttx/data/form/storage.h"
#include "ttx/semantic/transport/flow.h"

// Copy reports whether the promised observation was delivered. Counting the
// steps taken would make the result depend on how the provider fulfilled that
// promise. Failure can leave writes behind, but supplies no usable prefix or
// rollback guarantee. A policy that needs either must arrange it separately.
// Copy supplies a Storage view to an established Flow and records an observation
// of the source in that storage. On success, the result satisfies the source's
// promised wire format at that observation point.
//
// Copy supports all four Flow transports. Fragment observes values one at a
// time, so writes to overlapping storage can affect subsequent reads. Invocation
// order is well defined but the combined result is unspecified. A policy that
// needs a snapshot must arrange it explicitly, since Copy does not require one.
//
// This permits generators to fulfill Copy without retaining a source buffer.
// A random number generator can promise a representation of any supported size
// and produce each value when observed.
PERIMORTEM_C ttx_data_status ttx_copy(const ttx_flow* flow, ttx_storage target);

#endif
