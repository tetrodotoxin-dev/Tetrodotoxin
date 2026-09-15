// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_INTERFACE_PROVIDER_H
#define VALIDATION_INTERFACE_PROVIDER_H

#include "ttx/semantic/negotiation/query.h"

#define COUNTER_ID_HIGH ((U64)0x2d4fc0031eda431bULL)
#define COUNTER_ID_LOW ((U64)0xa69d57835fb2fd19ULL)

// The whole API crosses the binding boundary. Its receiver is an interior
// address of private C state; neither C++ nor a two-pointer envelope defines it.
typedef struct counter_api {
  const void* receiver;
  U64 (*add)(const void* receiver, U64 amount);
  U64 (*read)(const void* receiver);
} counter_api;

typedef struct counter_statistics {
  U64 queries;
  U64 calls;
} counter_statistics;

typedef struct counter_fixture {
  ttx_semantic_query query;
  void (*reset)(ttx_binding_status status, U8 omit, U8 stateless);
  counter_statistics (*statistics)(void);
} counter_fixture;

typedef ttx_data_status (*interface_compile)(
    ttx_schema_reference, ttx_representation_allocator, const ttx_representation**);
PERIMORTEM_C counter_fixture interface_provider_open(interface_compile compiler);

#endif
