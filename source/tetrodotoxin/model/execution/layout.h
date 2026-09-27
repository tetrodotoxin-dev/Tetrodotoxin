// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_EXECUTION_LAYOUT_H
#define TETRODOTOXIN_MODEL_EXECUTION_LAYOUT_H

#include "ttx/concept/abstract.h"

#define TETRODOTOXIN_MODEL_EXECUTION_LAYOUT_ID_HIGH 0x9d7a0ec6511647d0ULL
#define TETRODOTOXIN_MODEL_EXECUTION_LAYOUT_ID_LOW 0xa8ead4c0d6c40cdfULL

// An execution Layout orders semantic positions rather than bytes. Each subject
// carries its own value, Type or policy. The caller queries only indices below
// size,
// and every returned Abstract borrows the publication. Physical packing is a
// separate realization through Type Storage and TTX Data.
// This borrowed observation keeps its size and subjects stable while consumed.
typedef struct tetrodotoxin_model_execution_layout {
  const void* source;
  Count (*get_size)(const void* source);
  ttx_abstract (*get_subject)(const void* source, Count index);
} tetrodotoxin_model_execution_layout;

PERIMORTEM_C const ttx_representation*
tetrodotoxin_model_execution_layout_representation(void);

#endif
