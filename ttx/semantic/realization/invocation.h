// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_SEMANTIC_REALIZATION_INVOCATION_H
#define TTX_SEMANTIC_REALIZATION_INVOCATION_H

#include "ttx/semantic/negotiation/query.h"

// A consumer may know the data an operation accepts without having a native
// declaration for its function. Invocation lets the provider expose that
// operation through described input and output storage. Binding the operation's
// UUID checks this record's callable ABI. Its input and output descriptors then
// establish which payloads invoke can consume and produce.
//
// The record may be populated through Block without a persistent backing
// table. Receiver, frame descriptions and executable code borrow the enclosing
// publication. Copying the record acquires no additional ownership. An operation
// requiring independent release exposes that obligation in its own contract.
// Inputs and outputs are borrowed until the synchronous call returns, after
// which only success provides usable output. A policy offering asynchronous
// execution must establish its own lifetime for the operation's data.
typedef struct ttx_invocation {
  const void* receiver;
  const ttx_representation* inputs;
  const ttx_representation* outputs;
  ttx_data_status (*invoke)(const void* receiver, const void* inputs, void* outputs);
} ttx_invocation;

PERIMORTEM_C const ttx_representation* ttx_invocation_representation(void);

#endif
