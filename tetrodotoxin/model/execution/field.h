// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_EXECUTION_FIELD_H
#define TETRODOTOXIN_MODEL_EXECUTION_FIELD_H

#include "ttx/concept/abstract.h"

#define TETRODOTOXIN_MODEL_EXECUTION_FIELD_ID_HIGH 0x0cfb98d58afe4677ULL
#define TETRODOTOXIN_MODEL_EXECUTION_FIELD_ID_LOW 0x9bae95f2df125623ULL

// A field is one typed value position. Its Type edge retains the supplying
// policy rather than recovering a native Type object. Names and source spans
// belong to a declaration policy and are not required to use this position.
typedef struct tetrodotoxin_model_execution_field {
  const void* source;
  ttx_abstract (*get_type)(const void* source);
} tetrodotoxin_model_execution_field;

PERIMORTEM_C const ttx_representation*
tetrodotoxin_model_execution_field_representation(void);

#endif
