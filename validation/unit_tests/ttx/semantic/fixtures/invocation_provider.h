// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_INVOCATION_PROVIDER_H
#define VALIDATION_INVOCATION_PROVIDER_H

#include "ttx/semantic/realization/invocation.h"

#define INVOCATION_METHOD_HIGH 0x7a2c283c70c0447dULL
#define INVOCATION_METHOD_LOW 0x92230dc2db47d881ULL

// The mixed record deliberately falls outside the old list of native call
// signatures. Both languages consume this actual C type after form agreement.
typedef struct invocation_input {
  S64 value;
  R64 scale;
  U8 negate;
} invocation_input;

typedef struct invocation_statistics {
  U64 binds;
  U64 transfers;
  U64 calls;
  U64 acquisitions;
  U64 releases;
} invocation_statistics;

typedef struct invocation_fixture {
  ttx_semantic_query query;
  invocation_statistics (*statistics)(void);
  void (*configure)(U8 protocol, ttx_binding_status status, U8 fail_transfer);
} invocation_fixture;

PERIMORTEM_C invocation_fixture invocation_provider_open(
    const ttx_representation* record, const ttx_representation* inputs,
    const ttx_representation* outputs);

#endif
