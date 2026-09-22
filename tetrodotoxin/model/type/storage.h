// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_TYPE_STORAGE_H
#define TETRODOTOXIN_MODEL_TYPE_STORAGE_H

#include "ttx/concept/abstract.h"

#define TETRODOTOXIN_MODEL_TYPE_STORAGE_ID_HIGH 0xba32a6ca9b7040c5ULL
#define TETRODOTOXIN_MODEL_TYPE_STORAGE_ID_LOW 0x9b1caa1947f482ffULL

// A Type can admit storage without depending on an execution graph. The returned
// Representation describes this publication's selected realization. It borrows
// the provider, which can decline or defer the question without fabricating
// byte geometry. A different target policy can supply a different realization.
typedef struct tetrodotoxin_model_type_storage {
  const void* source;
  ttx_binding_status (*get_representation)(const void* source, const ttx_representation** output);
} tetrodotoxin_model_type_storage;

PERIMORTEM_C const ttx_representation*
tetrodotoxin_model_type_storage_representation(void);

#endif
