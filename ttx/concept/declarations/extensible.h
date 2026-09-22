// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_DECLARATIONS_EXTENSIBLE_H
#define TTX_CONCEPT_DECLARATIONS_EXTENSIBLE_H

#include "ttx/data/status.h"
#include "ttx/semantic/ownership/publication.h"

#define TTX_EXTENSIBLE_ID_HIGH 0x84c6c053b3254f7aULL
#define TTX_EXTENSIBLE_ID_LOW 0xa873409b31bcf604ULL

// Extensible marks a constructible object surface. Its Abstract's visible
// members describe the available operations through their own contracts.
// Emission supplies a publication that binds Factory and owns the runtime
// context needed to construct instances. That context must remain valid after
// the declaration graph is released.
//
// A host policy can compile this surface into another runtime's class factory.
// Neither the declaration nor the emitted factory needs that runtime's native
// class representation. Success transfers the factory publication, and failure
// transfers nothing. Calls and discovery are synchronous.
typedef struct ttx_extensible_operations {
  ttx_data_status (*emit_factory)(const void* source, ttx_publication* output);
} ttx_extensible_operations;

typedef struct ttx_extensible {
  const void* source;
  const ttx_extensible_operations* operations;
} ttx_extensible;

PERIMORTEM_C const ttx_representation* ttx_extensible_representation(void);

#endif
