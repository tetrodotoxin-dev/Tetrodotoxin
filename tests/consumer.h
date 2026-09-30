// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_TEST_CONSUMER_H
#define TETRODOTOXIN_TEST_CONSUMER_H

#include "tetrodotoxin/dialects/source/input.h"
#include "ttx/concept/policies/borrowed.h"
#include "ttx/semantic/negotiation/query.h"

// The C observer retains its answer beyond the loader's query callback. The
// C++ host lends public representations and input bytes through open, then
// overwrites those bytes before finish checks the retained result. It keeps
// code loaded until this observer has released that result.
typedef struct source_consumer {
  const ttx_representation* input;
  const ttx_representation* stream;
  const ttx_representation* token;
  ttx_borrowed retained;
  int failed;
} source_consumer;

PERIMORTEM_C ttx_binding_status source_consumer_open(
    source_consumer* consumer,
    ttx_semantic_query query,
    const tetrodotoxin_source_input* input);
PERIMORTEM_C int source_consumer_finish(source_consumer* consumer);

#endif
