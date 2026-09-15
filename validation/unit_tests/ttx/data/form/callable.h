// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_TTX_DATA_CALLABLE_H
#define VALIDATION_TTX_DATA_CALLABLE_H

#include "ttx/data/form/schema.h"

// The native vector is deliberately distinct from Fragment's output carrier.
// A callable passes it in the SIMD registers promised by its descriptor.
typedef R32 ttx_test_vector __attribute__((vector_size(16)));
typedef struct ttx_test_callables {
  U32 (*sum)(U32, U32, U32, U32);
  R64 (*variadic)(U32, ...);
  ttx_test_vector (*twice)(ttx_test_vector);
} ttx_test_callables;

PERIMORTEM_C const ttx_schema* ttx_test_callable_schema(void);
PERIMORTEM_C const void* ttx_test_callable_table(void);

#endif
