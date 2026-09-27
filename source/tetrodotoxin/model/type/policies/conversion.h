// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_TYPE_CONVERSION_H
#define TETRODOTOXIN_MODEL_TYPE_CONVERSION_H

#include "ttx/concept/abstract.h"

#define TETRODOTOXIN_MODEL_TYPE_CONVERSION_ID_HIGH 0x7d27bdf063bf4aa4ULL
#define TETRODOTOXIN_MODEL_TYPE_CONVERSION_ID_LOW 0x8f50e8f0c309c715ULL

// A receiving Type owns which value observations it can accept. Conversion
// asks that policy for the actual source subject in this transaction. Success
// returns the admitted producer, which may be the source or a provider owned
// projection. No write is implied, and the result grants no permission for a
// later transaction. Retaining an answer without repeating this question needs
// a Constant proof for the particular edge, not merely a retained interface.
//
// Inputs and returned subjects borrow their publications. A provider supplying
// a new projection keeps it alive under that publication's lifetime. Unsupported,
// Pending and Rejected leave output unavailable and preserve the current policy.
typedef struct tetrodotoxin_model_type_conversion {
  const void* source;
  ttx_binding_status (*convert)(const void* source, ttx_abstract value, ttx_abstract* output);
} tetrodotoxin_model_type_conversion;

PERIMORTEM_C const ttx_representation* tetrodotoxin_model_type_conversion_representation(void);

#endif

