// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_THUNK_PROVIDER_H
#define VALIDATION_THUNK_PROVIDER_H

#include "ttx/semantic/query.h"
#include "ttx/semantic/thunk.h"

#define COUNTER_ID_HIGH ((U64)0x2d4fc0031eda431bULL)
#define COUNTER_ID_LOW ((U64)0xa69d57835fb2fd19ULL)

// The public promise is synchronous accumulation with an explicit opaque self.
// Its native state is private to the C module, including the projection used
// as the receiver. Test controls observe negotiation separately from calls.
typedef struct counter_operations {
  U64 (*add)(const void* self, U64 amount);
} counter_operations;

typedef struct counter_statistics {
  U64 queries;
  U64 fulfillments;
  U64 calls;
} counter_statistics;

typedef struct counter_fixture {
  ttx_semantic_query query;
  void (*reset)(ttx_binding_status query_status,
                ttx_binding_status fulfillment_status, U8 omit, U8 stateless);
  counter_statistics (*statistics)(void);
} counter_fixture;

PERIMORTEM_C counter_fixture thunk_provider_open(const ttx_representation*);

#endif
