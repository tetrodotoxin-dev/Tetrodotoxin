// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_EXECUTION_VALUE_H
#define TETRODOTOXIN_MODEL_EXECUTION_VALUE_H

#include "ttx/semantic/negotiation/query.h"

#define TETRODOTOXIN_MODEL_EXECUTION_VALUE_ID_HIGH 0x7d6713abbb6e433dULL
#define TETRODOTOXIN_MODEL_EXECUTION_VALUE_ID_LOW 0x9caa7b3f1c334db7ULL

// A value observation can be backed by a buffer, device, generator or remote
// service. Value supplies the data Query for that observation without exposing
// its storage mechanism. Domain separately supplies its current Type. The
// returned Query borrows the publication, and repeated data observations may
// differ unless the producer supplies the Execution Constant promise.
typedef struct tetrodotoxin_model_execution_value {
  const void* source;
  ttx_semantic_query (*get_value)(const void* source);
} tetrodotoxin_model_execution_value;

PERIMORTEM_C const ttx_representation* tetrodotoxin_model_execution_value_representation(void);

#endif

