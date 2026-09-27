// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_EXECUTION_ASSIGNMENT_H
#define TETRODOTOXIN_MODEL_EXECUTION_ASSIGNMENT_H

#include "ttx/concept/abstract.h"

#define TETRODOTOXIN_MODEL_EXECUTION_ASSIGNMENT_ID_HIGH 0x9bbb593537b143bbULL
#define TETRODOTOXIN_MODEL_EXECUTION_ASSIGNMENT_ID_LOW 0x92fb3ce3970df90aULL

// Write authority belongs to the encountered destination. A retained binding
// can remain callable while a private or stage policy changes its answer, so
// every assign call asks about its actual source in the current transaction.
// A destination may convert that value before writing without making conversion
// itself grant write access.
//
// The returned binding status describes admission. Only Satisfied supplies a
// transfer outcome, which reports whether the admitted write completed. This
// keeps Pending or a policy refusal distinct from an I/O failure after a write
// began. A failed transfer supplies no valid result and may have touched the
// destination. Atomic commit or rollback belongs to the destination policy.
typedef struct tetrodotoxin_model_execution_assignment {
  const void* source;
  ttx_binding_status (*assign)(const void* source, ttx_abstract value, ttx_data_status* outcome);
} tetrodotoxin_model_execution_assignment;

PERIMORTEM_C const ttx_representation* tetrodotoxin_model_execution_assignment_representation(void);

#endif

