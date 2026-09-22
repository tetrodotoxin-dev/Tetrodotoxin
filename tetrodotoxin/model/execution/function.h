// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_EXECUTION_FUNCTION_H
#define TETRODOTOXIN_MODEL_EXECUTION_FUNCTION_H

#include "tetrodotoxin/model/execution/layout.h"

#define TETRODOTOXIN_MODEL_EXECUTION_FUNCTION_ID_HIGH 0xda1e689e155349a4ULL
#define TETRODOTOXIN_MODEL_EXECUTION_FUNCTION_ID_LOW 0x81b4431a99faa267ULL

// A function publishes semantic parameters, results and its execution body.
// Its operation identity names the behavior later realized by a terminal.
// The body may be Unknown while a supplying policy is still interpreting it.
// Binding this description neither invokes the function nor requires source
// provenance. All returned edges borrow the declaration publication.
typedef struct tetrodotoxin_model_execution_function {
  const void* source;
  perimortem_uuid (*get_operation)(const void* source);
  tetrodotoxin_model_execution_layout (*get_parameters)(const void* source);
  tetrodotoxin_model_execution_layout (*get_results)(const void* source);
  ttx_abstract (*get_body)(const void* source);
} tetrodotoxin_model_execution_function;

PERIMORTEM_C const ttx_representation*
tetrodotoxin_model_execution_function_representation(void);

#endif
