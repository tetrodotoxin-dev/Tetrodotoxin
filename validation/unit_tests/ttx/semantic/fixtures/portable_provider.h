// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_PORTABLE_PROVIDER_H
#define VALIDATION_PORTABLE_PROVIDER_H

#include "ttx/semantic/negotiation/query.h"

// This small API crosses the same negotiated boundary in a native executable
// and in WebAssembly. The C owner publishes literal descriptor bytes so the
// check can catch a disagreement with the C++ compiler's canonical encoding.
typedef struct portable_counter {
  const void* receiver;
  U32 (*add)(const void* receiver, U32 amount);
} portable_counter;

PERIMORTEM_C ttx_semantic_query portable_counter_open(void);
PERIMORTEM_C U32 portable_counter_calls(void);

#endif
