// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_EXECUTION_RETURN_H
#define TETRODOTOXIN_MODEL_EXECUTION_RETURN_H

#include "tetrodotoxin/model/execution/layout.h"

#define TETRODOTOXIN_MODEL_EXECUTION_RETURN_ID_HIGH 0x30a084d0828345d6ULL
#define TETRODOTOXIN_MODEL_EXECUTION_RETURN_ID_LOW 0x8381c10c688846b5ULL

// Return transfers its ordered values to the enclosing function's results.
// An empty Layout returns no values. The terminal checks the corresponding
// Types before emitting code. No source statement or lexical context is needed.
typedef struct tetrodotoxin_model_execution_return {
  const void* source;
  tetrodotoxin_model_execution_layout (*get_values)(const void* source);
} tetrodotoxin_model_execution_return;

PERIMORTEM_C const ttx_representation*
tetrodotoxin_model_execution_return_representation(void);

#endif
