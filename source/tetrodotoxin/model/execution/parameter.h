// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_EXECUTION_PARAMETER_H
#define TETRODOTOXIN_MODEL_EXECUTION_PARAMETER_H

#include "ttx/concept/abstract.h"

#define TETRODOTOXIN_MODEL_EXECUTION_PARAMETER_ID_HIGH 0xdc2b847e2fc64751ULL
#define TETRODOTOXIN_MODEL_EXECUTION_PARAMETER_ID_LOW 0xa51136cc914e4b6aULL

// A parameter use refers to the exact declared field of its function. Keeping
// that edge lets a terminal connect a use to its input without reading names
// or assuming a native parameter class. The field retains its original policy.
typedef struct tetrodotoxin_model_execution_parameter {
  const void* source;
  ttx_abstract (*get_field)(const void* source);
} tetrodotoxin_model_execution_parameter;

PERIMORTEM_C const ttx_representation*
tetrodotoxin_model_execution_parameter_representation(void);

#endif
