// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_TYPE_POLICIES_FLAG_H
#define TETRODOTOXIN_MODEL_TYPE_POLICIES_FLAG_H

#include "ttx/data/form/storage.h"

#define TETRODOTOXIN_MODEL_TYPE_FLAG_ID_HIGH 0x451632406ee44886ULL
#define TETRODOTOXIN_MODEL_TYPE_FLAG_ID_LOW 0xb7ffea2a011adc90ULL

// Truth is a semantic observation of a value, not a consequence of its size.
// The caller first obtains an admitted value through its chosen transport.
// Flag then interprets that observation under the supplying type's rules.
// Input bytes are borrowed only for this call and are never modified. Success
// writes zero or one, while failure leaves the output unavailable.
typedef struct tetrodotoxin_model_type_flag {
  const void* source;
  ttx_data_status (*get_truth)(const void* source, ttx_storage value, U8* output);
} tetrodotoxin_model_type_flag;

#endif
