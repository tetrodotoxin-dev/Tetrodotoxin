// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_SEMANTIC_OWNERSHIP_FACTORY_H
#define TTX_SEMANTIC_OWNERSHIP_FACTORY_H

#include "ttx/data/status.h"
#include "ttx/semantic/ownership/publication.h"

#define TTX_FACTORY_ID_HIGH 0x84c6c053b3254f7aULL
#define TTX_FACTORY_ID_LOW 0xa873409b31bcf602ULL

// An emitted factory creates runtime publications without reconstructing its
// source graph. Each successful call transfers an independent instance Query.
// The consumer acquires its callable bindings during construction and retains
// that instance through their use. Failure transfers nothing.
//
// The factory publication owns its runtime context independently of discovery.
// Retain it through instance release, since an instance may borrow shared
// factory context instead of copying that context into every receiver.
// Its enclosing code owner remains alive through factory and instance release.
typedef struct ttx_factory_operations {
  ttx_data_status (*create)(const void* source, ttx_publication* output);
} ttx_factory_operations;

typedef struct ttx_factory {
  const void* source;
  const ttx_factory_operations* operations;
} ttx_factory;

PERIMORTEM_C const ttx_representation* ttx_factory_representation(void);

#endif
